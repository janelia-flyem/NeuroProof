/*!
 * \file
 * The following program is used to analyze a segmentation graph.
 * When a segmentation is constructed for a image volume, each of
 * the regions in segmentation can be represented as nodes in a
 * graph and the adjancency of these regions as edges.  This program
 * assumes that each edge has some confidence associated with it
 * to indicate the likelihood of the edge being a true or false edge.
 * With this information, analyze_segmentation_graph_gt can use ground
 * truth to determine its similarity and how much work is required to 
 * correct errors in the segmentation graph.  This file is similar
 * to the anaylze_segmentation_graph except that results are tied to
 * ground truth rather than just being estimates.  Most of this work
 * assumes that the segmentation volume is an over segmentation of the
 * ground truth.  If there is under segmentation, the analysis contained
 * here will be of less value since edit distance is more accurately
 * measured in terms of merges to be made.
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

// controller for stack containing label volume and rag
#include "BioPriors/BioStackController.h"

// contains library for storing a region adjacency graph (RAG)
#include "Rag/Rag.h"

// algorithms used to estimate how many edges need to be examine
#include "Priority/LocalEdgePriority.h"

// simple function for measuring runtime
#include "Utilities/ScopeTime.h"

// utilties for importing rags
#include "Rag/RagIO.h"

// utitlies for parsing options
#include "Utilities/OptionParser.h"

// utility to read label volumes
#include "Utilities/h5read.h"

#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <json/json.h>
#include <json/value.h>

using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;
using namespace NeuroProof;

using namespace boost::program_options;
using std::tr1::unordered_map; 
using std::tr1::unordered_set;
using std::exception;

// path to label in h5 file
static const char * SEG_DATASET_NAME = "stack";

// padding around images
static const int PADDING = 1;

typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

/* Container for analysis options
 * The basic options are to compute Variance of Information statistics
 * against ground truth.  This analysis can show the bodies that are
 * the largest violators.  A metric of edit distance shows the number of
 * 'fixes' required required to make both label volumes 'equal' and the
 * percentage of volume that changes.  The options also allow a user to specify
 * a set of actions that can be performed to improve the segmentation volume (the
 * types of actions will depend on the underlying functions supported).
 * If these actions are performed, the amount of work and VI achieved is reported.
 * Regardless of editing performed, VI and a report on types of discrepancies
 * are provided on the final segmentation volume.  This analysis is performed
 * over the synapse graph as well (including edit distance eventually) if a
 * synapse file is provided.
*/
struct AnalyzeGTOptions
{
    /*!
     * Loads and parses options
     * \param argc Number of arguments in passed array
     * \param argv Array of strings
    */
    AnalyzeGTOptions(int argc, char** argv) : gt_dilation(1), seg_dilation(0),
    dump_split_merge_bodies(false), dump_orphans(false), vi_threshold(0.02), synapse_filename(""),
    clear_synapse_exclusions(false), body_error_size(25000), synapse_error_size(1),
    graph_filename(""), exclusions_filename(""), recipe_filename(""), min_filter_size(0),
    random_seed(1) 
    {
        OptionParser parser("Program analyzes a segmentation graph with respect to ground truth");

        // positional arguments
        parser.add_positional(seg_filename, "seg-file",
                "h5 file with segmentation label volume (z,y,x) and body mappings"); 
        parser.add_positional(groundtruth_filename, "groundtruth-file",
                "h5 file with groundtruth label volume (z,y,x) and body mappings"); 

        // optional arguments
        parser.add_option(gt_dilation, "gt-dilation",
                "Dilation factor for the ground truth volume boundaries (>1 not supported)"); 
        parser.add_option(seg_dilation, "seg-dilation",
                "Dilation factor for the segmentation volume boundaries (>1 not supported)"); 
        parser.add_option(dump_split_merge_bodies, "dump-split-merge-bodies",
                "Output all large VI differences at program completion"); 
        parser.add_option(dump_orphans, "dump-orphans",
                "Output all orphans that cannot be obviously matched between the volumes"); 
        parser.add_option(vi_threshold, "vi-threshold",
                "Threshold at which VI differences are reported"); 
        parser.add_option(synapse_filename, "synapse-file",
                "json file containing synapse information for label volumes (enable synapse analysis)"); 
        parser.add_option(clear_synapse_exclusions, "clear-synapse-exclusions",
                "Ignore synapse-based exclusions when agglomerating"); 
        parser.add_option(body_error_size, "body-error-size",
                "Threshold above which errors are analyzed"); 
        parser.add_option(synapse_error_size, "synapse-error-size",
                "Threshold above which synapse errors are analyzed"); 
        parser.add_option(graph_filename, "graph-file",
                "json file that sets edge probabilities (default is optimal) and synapse constraints"); 
        parser.add_option(exclusions_filename, "exclusions-file",
                "json file that specifies bodies to ignore during VI"); 
        parser.add_option(recipe_filename, "recipe-file",
                "json file that specifies editing operations to be performed automatically"); 
        parser.add_option(min_filter_size, "filter-size",
                "body size filter below which bodies are ignored in VI computation -- should not run with synapse VI"); 
    
        // invisible arguments
        parser.add_option(random_seed, "random-seed",
                "seed used for randomizing recipe", true, false, true); 

        parser.parse_options(argc, argv);
    }

    // manadatory positionals -- basic VI comparison
    string seg_filename;
    string groundtruth_filename;

    // optional (with default values) 
    int gt_dilation; //! dilation of ground truth boundaries
    int seg_dilation; //! dilation of segmentation volume boundaries
    bool dump_split_merge_bodies; //! dump all body differences at the end
    bool dump_orphans; //! dump all orphans that are not matched between seg and gt
    double vi_threshold; //! below which body differences are not reported

    /*!
     * Synapse json file that contains all synapses in the volume.
     * If specified, a synapse label to body label array is made where a contigency
     * table can be made just as in the body VI mode.  If there is no
     * graph file, exclusions are automatically set in the main stack and
     * a synapse list is given to the priority algorithm. 
    */
    string synapse_filename;
    bool clear_synapse_exclusions; //! erase all synapse exlusions during auto edit

    /*! 
      * Threshold above which errors are analyzes (agglomerate nodes below
      * this size until above the threshold if possible and record this
      * error as a special type of large body error that is perhaps unavoidable
      * in path based probability analysis).  This also includes noting the edge
      * and affinity status (if available) between large error bodies.
      * On the over-merge size, the number of larger-than-threshold over-merges
      * are noted.  Orphans of size threshold are reported.
    */
    int body_error_size;
    int synapse_error_size; //! same as with body error size (if available)

    /*!
     * Provides information on uncertainty on edges.  If there is no graph,
     * one is automatically created and edge probs are set by optimal.  For now,
     * the graph must contain synapse information if the synapse priority modes
     * are to be run.
    */ 
    string graph_filename; 
   
    string exclusions_filename; //! file that contains all bodies to ignore
     
    /*!
     * Contains operations to try and whether to use body restrictions.  For each
     * editing actions, the number of operations and number of merges are noted.
     * VI is reported after each item in the recipe.  Detailed error analysis
     * is done at the end of all the actions. */ 
    string recipe_filename; //! contains operations to try and body restrictions
   
    //! body size below which is ignored
    unsigned long long min_filter_size;
    
    // hidden option (with default value)
    int random_seed;
};

int num_synapse_errors(BioStackController& controller, int threshold)
{
    Stack2* stack = controller.get_stack();
    RagPtr rag = stack->get_rag();
    unordered_map<Label_t, int> synapse_counts;
    controller.load_synapse_counts(synapse_counts);
    int synapse_errors = 0;
    for (unordered_map<Label, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter) {
        if (iter->second >= threshold) {
            RagNode_uit* node = rag->find_rag_node(iter->first);
            if (!(node->is_boundary())) {
                ++synapse_errors;
            }
        }
    }
    return synapse_errors;
}

int num_body_errors(StackController& controller, int threshold)
{
    Stack2* stack = controller.get_stack();
    RagPtr rag = stack->get_rag();
    int body_errors = 0;
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if ((!((*iter)->is_boundary())) && ((*iter)->get_size() >= threshold)) {
            ++body_errors;
        }
    }
    return body_errors;
}


void load_orphans(BioStackController& controller, unordered_set<Label>& orphans,
        int threshold, int synapse_threshold)
{
    unordered_map<Label, int> synapse_counts;
    controller.load_synapse_counts(synapse_counts);
    RagPtr rag = controller.get_stack()->get_rag();

    for (unordered_map<Label, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter) {
        if (iter->second >= synapse_threshold) {
            RagNode_uit* node = rag->find_rag_node(iter->first);
            if (!(node->is_boundary())) {
                orphans.insert(iter->first);
            }
        }
    }

    for (Rag_uit::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if ((!((*iter)->is_boundary())) && ((*iter)->get_size() >= threshold)) {
            orphans.insert((*iter)->get_node_id());
        }
    }
}

void dump_vi_differences(StackController& controller, double threshold)
{
    double merge, split;
    multimap<double, Label_t> label_ranked;
    multimap<double, Label_t> gt_ranked;
    controller.compute_vi(merge, split, label_ranked, gt_ranked);

    cout << "Dumping VI differences to threshold: " << threshold << " where MergeSplit: ("
        << merge << ", " << split << ")" << endl;

    double totalS = 0.0;
    double totalG = 0.0;
    cout << endl << "******Seg Merged Bodies*****" << endl;
    for (multimap<double, Label>::reverse_iterator iter = label_ranked.rbegin();
            iter != label_ranked.rend(); ++iter) {
        if (merge < threshold) {
            break;
        }
        cout << iter->second << " " << iter->first << endl;
        merge -= iter->first;
    }
    cout << endl; 

    cout << endl << "******GT Merged Bodies*****" << endl;
    for (multimap<double, Label>::reverse_iterator iter = gt_ranked.rbegin();
            iter != gt_ranked.rend(); ++iter) {
        if (split < threshold) {
            break;
        }
        cout << iter->second << " " << iter->first << endl;
        split -= iter->first;
    }
    cout << endl; 

} 

void get_num_edits(LocalEdgePriority& priority_scheduler, StackController controller,
        StackController synapse_controller, RagPtr opt_rag, int& num_modified,
        int& num_examined, bool use_exclusions)
{
    int edges_examined = 0;
    int num_exclusions = 0;
    RagPtr rag = controller.get_stack()->get_rag();
    LowWeightCombine join_alg;

    while (!priority_scheduler.isFinished()) {
        EdgePriority::Location location;

        // choose most impactful edge given pre-determined strategy
        boost::tuple<Label, Label> pair = priority_scheduler.getTopEdge(location);
        
        Label_t node1 = boost::get<0>(pair);
        Label_t node2 = boost::get<1>(pair);
        RagEdge_uit* temp_edge = opt_rag->find_rag_edge(node1, node2);
        double weight = temp_edge->get_weight();

        bool merged = false;
        // use optimal prediction for decision
        if (weight < 0.00001) {
            ++num_modified;
            priority_scheduler.removeEdge(pair, true);
            // update rags and watersheds as appropriate
            rag_join_nodes(*opt_rag, opt_rag->find_rag_node(node1),
                    opt_rag->find_rag_node(node2), &join_alg); 
            controller.merge_labels(node2, node1, &join_alg, true);
            // no rag maintained for synapse stack
            if (synapse_controller.get_stack()->get_labelvol()) {
                synapse_controller.merge_labels(node2, node1, &join_alg, true);
            } 
            merged = true;
        } else {
            priority_scheduler.removeEdge(pair, false);
        }
       
        ++num_examined;
    }
}

void dump_differences(BioStackController& controller, StackController& synapse_controller,
        BioStackController& gt_controller, RagPtr opt_rag, AnalyzeGTOptions& options)
{
    RagPtr seg_rag = controller.get_stack()->get_rag();
    RagPtr gt_rag = gt_controller.get_stack()->get_rag();

   
    cout << "************Printing Statistics**************" << endl; 
    cout << "Graph edges: " << seg_rag->get_num_edges() << endl;
    cout << "Graph nodes: " << seg_rag->get_num_regions() << endl;

    cout << "Body VI: ";    
    double merge, split;
    controller.compute_vi(merge, split);
    cout << "MergeSplit: (" << merge << ", " << split << ")" << endl; 

    // find body errors
    int body_errors = num_body_errors(controller, options.body_error_size);
    cout << "Number of orphan bodies with more than " << options.body_error_size << 
        " voxels : " << body_errors << endl;

    if (options.synapse_filename != "") {
        cout << "Synapse VI: ";
        synapse_controller.compute_vi(merge, split);
        cout << "MergeSplit: (" << merge << ", " << split << ")" << endl; 

        // find synapse errors 
        int synapse_errors = num_synapse_errors(controller, options.synapse_error_size); 
        cout << "Number of orphan bodies with more than " << options.synapse_error_size << 
            " synapse annotations : " << synapse_errors << endl;   
    }
    
    unordered_set<Label> seg_orphans; 
    unordered_set<Label> gt_orphans; 
    
    load_orphans(controller, seg_orphans, options.body_error_size,
            options.synapse_error_size);
    load_orphans(gt_controller, gt_orphans, options.body_error_size,
            options.synapse_error_size);

    unordered_set<Label> seg_matched; 
    unordered_set<Label> gt_matched; 
    int matched = 0;
    for (std::tr1::unordered_set<Label>::iterator iter = seg_orphans.begin();
           iter != seg_orphans.end(); ++iter) {
        matched += controller.match_regions_overlap((*iter), gt_orphans,
                gt_rag, seg_matched, gt_matched);
    } 
    
    cout << "Percent orphans matched for seg: " <<
        seg_matched.size()/double(seg_orphans.size())*100 << endl;
    if (options.dump_orphans) {
        for (std::tr1::unordered_set<Label>::iterator iter = seg_orphans.begin();
                iter != seg_orphans.end(); ++iter) {
            if (seg_matched.find(*iter) == seg_matched.end()) {
                cout << "Unmatched seg orphan body: " << *iter << endl;
            } 
        }
    }
    cout << "Percent orphans matched for GT: " <<
        double(gt_matched.size())/gt_orphans.size()*100 << endl;
    
    if (options.dump_orphans) {
        for (std::tr1::unordered_set<Label>::iterator iter = gt_orphans.begin();
                iter != gt_orphans.end(); ++iter) {
            if (gt_matched.find(*iter) == gt_matched.end()) {
                cout << "Unmatched GT orphan body: " << *iter << endl;
            } 
        }
    }
    int leftover = seg_orphans.size() - seg_matched.size() + gt_orphans.size() - gt_matched.size();
    cout << "Orphan agreement rate: " << double(matched)/(leftover + matched) * 100.0 <<  endl; 
     
    // might give weird results when dilation is done on GT ??
    controller.compute_groundtruth_assignment();
    vector<RagNode_uit* > large_bodies;

    for (Rag_uit::nodes_iterator iter = seg_rag->nodes_begin();
           iter != seg_rag->nodes_end(); ++iter) {
        if ((*iter)->get_size() > options.body_error_size) {
            large_bodies.push_back((*iter));
        }
    }

    int num_unmerged = 0;
    Json::Value json_vals;
    //LocalEdgePriority<Label> temp_scheduler(*seg_rag, 0.1, 0.9, 0.1, json_vals);
    for (int i = 0; i < large_bodies.size(); ++i) {
        for (int j = (i+1); j < large_bodies.size(); ++j) {
            int val = controller.find_edge_label(large_bodies[i]->get_node_id(),
                    large_bodies[j]->get_node_id());

            if (val == -1) {
                ++num_unmerged;
                /*double pathprob = temp_scheduler.find_path(large_bodies[i], large_bodies[j]);
                cout << "prob: " << pathprob << " between " <<
                    large_bodies[i]->get_size() << " and " <<
                    large_bodies[j]->get_size() << endl; */
            }
        }
    }
   
    // TODO: should really print distribution of bodies unmerged at different
    // thresholds, not necessarily tied to focused proofreading threshold 
    cout << "Number of unmerged pairs of large bodies: " << num_unmerged << 
        " from " << large_bodies.size() << " large bodies" << endl;

    // TODO get affinity pairs for missed node pairs
    

    std::tr1::unordered_map<Label, unsigned long long> body_changes;
    Rag_uit opt_rag_copy(*opt_rag);

    bool change = true;
    LowWeightCombine join_alg;
    while (change) {
        change = false;
        for (Rag_uit::edges_iterator iter = opt_rag_copy.edges_begin();
                iter != opt_rag_copy.edges_end(); ++iter) {
            double weight = (*iter)->get_weight();
            if (weight < 0.0001) {
                RagNode_uit* node1 = (*iter)->get_node1();
                RagNode_uit* node2 = (*iter)->get_node2();

                unsigned long long large1 = node1->get_size();
                unsigned long long large2 = node2->get_size();
                if (body_changes.find(node1->get_node_id()) != body_changes.end()) {
                    large1 = body_changes[node1->get_node_id()];
                }
                if (body_changes.find(node2->get_node_id()) != body_changes.end()) {
                    large2 = body_changes[node2->get_node_id()];
                }
                RagNode_uit* node_keep;
                RagNode_uit* node_remove;
                if (large1 > large2) {
                    body_changes[node1->get_node_id()] = large1;
                    node_keep = node1;
                    node_remove = node2;
                    body_changes.erase(node2->get_node_id());
                } else {
                    body_changes[node2->get_node_id()] = large2;
                    node_keep = node2;
                    node_remove = node1;
                    body_changes.erase(node1->get_node_id());
                }

                vector<string> property_names;
                rag_join_nodes(opt_rag_copy, node_keep, node_remove, &join_alg); 

                change = true;
                break;
            }
        }
    }
    int num_undermerged_bodies = 0;
    for (std::tr1::unordered_map<Label, unsigned long long>::iterator iter = body_changes.begin();
            iter != body_changes.end(); ++iter) {
        unsigned long long largest_orig = iter->second;
        RagNode_uit* node1 = opt_rag_copy.find_rag_node(iter->first);
        // TODO: should print size distribution since there is really nothing missed
        // higher above the threshold
        if ((node1->get_size() - largest_orig) >= options.body_error_size) {
            ++num_undermerged_bodies;
        } 
    }
    cout << "Number of under-merged bodies: " << num_undermerged_bodies << endl;
   
    // TODO 
    // count number of bodies where largest component body and size is greater than threshold
    // percentage of total size >25k, percentage misassigned and orphan (same with synapses) 
    // print bodies (on both sides) that are off by threshold size
    // print number of body/synapse errors above size threshold -- print path affinity/note exclusions
    // print number of body/synapse errors that cumulatively cause problems
    // print number of VI violators to threshold and root cause by affinity?
    cout << "************Finished Printing Statistics**************" << endl; 
}


void run_body(LocalEdgePriority& priority_scheduler, Json::Value json_vals,
        StackController& controller, StackController& synapse_controller,
        RagPtr opt_rag, int& num_modified, int& num_examined)
{
    unsigned int path_length = json_vals.get("path-length", 0).asUInt(); 
    unsigned int threshold = json_vals.get("threshold", 25000).asUInt();
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;

    cout << endl << "Starting body mode with threshold " << threshold << " and path length "
        << path_length << endl;

    priority_scheduler.set_body_mode(threshold, path_length);

    get_num_edits(priority_scheduler, controller, synapse_controller, opt_rag,
            num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Body mode finished ... "; 
}

void run_synapse(LocalEdgePriority& priority_scheduler, Json::Value json_vals,
        StackController& controller, StackController& synapse_controller,
        RagPtr opt_rag, int& num_modified, int& num_examined)
{
    double threshold = json_vals.get("threshold", 0.1).asDouble();
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;

    cout << endl << "Starting synapse mode with threshold " << threshold << endl;

    priority_scheduler.set_synapse_mode(threshold);

    get_num_edits(priority_scheduler, controller, synapse_controller, opt_rag,
            num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Synapse mode finished ... "; 
}

void run_orphan(LocalEdgePriority& priority_scheduler, Json::Value json_vals,
        StackController& controller, StackController& synapse_controller,
        RagPtr opt_rag, int& num_modified, int& num_examined)
{
    unsigned int threshold = json_vals.get("threshold", 25000).asUInt();
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;
    cout << endl << "Starting orphan mode with threshold " << threshold << endl;

    priority_scheduler.set_orphan_mode(threshold, 0, 0);

    get_num_edits(priority_scheduler, controller, synapse_controller, opt_rag,
            num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Orphan mode finished ... "; 
}

void run_edge(LocalEdgePriority& priority_scheduler, Json::Value json_vals,
        StackController& controller, StackController& synapse_controller,
        RagPtr opt_rag, int& num_modified, int& num_examined)
{
    double lower = json_vals.get("lower-prob", 0.0).asDouble();
    double upper = json_vals.get("upper-prob", 0.9).asDouble();
    double starting = json_vals.get("starting-prob", 0.0).asDouble();
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;
    cout << endl << "Starting edge mode with lower threshold " << lower << 
        " and upper threshold " << upper << " and starting threshold " << starting << endl;

    priority_scheduler.set_edge_mode(lower, upper, starting);

    get_num_edits(priority_scheduler, controller, synapse_controller, opt_rag,
            num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Edge mode finished ... "; 
}

void set_rag_probs(StackController& controller, RagPtr rag) {
    for (Rag_uit::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        int val = controller.find_edge_label((*iter)->get_node1()->get_node_id(),
                (*iter)->get_node2()->get_node_id()); 
        if (val == -1) {
            (*iter)->set_weight(0.0);
        } else {
            (*iter)->set_weight(1.0);
        }
    }
}

void run_recipe(string recipe_filename, BioStackController& controller,
        StackController& synapse_controller, BioStackController& gt_controller,
        RagPtr opt_rag, AnalyzeGTOptions& options)
{
    // open json file
    Json::Reader json_reader;
    Json::Value json_vals;
    ifstream fin(recipe_filename.c_str());
    if (!fin) {
        throw ErrMsg("Error: input file: " + recipe_filename + " cannot be opened");
    }
    if (!json_reader.parse(fin, json_vals)) {
        throw ErrMsg("Error: Json incorrectly formatted");
    }
    fin.close();

    RagPtr rag = controller.get_stack()->get_rag();

    Json::Value json_vals_priority;
    
    // load orphan information
    Json::Value orphan_bodies;
    unsigned int id = 0;
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if (!((*iter)->is_boundary())) {
            orphan_bodies[id++] = (*iter)->get_node_id(); 
        }
    }
    
    // load default information
    for (Rag_uit::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        (*iter)->set_property("location", Location(0,0,0));
        (*iter)->set_property("edge_size", (*iter)->get_size());
    }
    
    json_vals_priority["orphan_bodies"] = orphan_bodies;

    // load synapse information
    unordered_map<Label, int> synapse_counts;
    controller.load_synapse_counts(synapse_counts);
    id = 0;
    Json::Value synapses;
    for (std::tr1::unordered_map<Label, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter) {
        Json::Value val_pair;
        val_pair[(unsigned int)(0)] = iter->first;
        val_pair[(unsigned int)(1)] = iter->second;
        synapses[id++] = val_pair;
    }
    json_vals_priority["synapse_bodies"] = synapses;

    LocalEdgePriority priority_scheduler(*rag, 0.1, 0.9, 0.1, json_vals_priority);
    
    int num_examined = 0;
    int num_modified = 0;

    for (int i = 0; i < json_vals["recipe"].size(); ++i) {
        Json::Value operation = json_vals["recipe"][i]; 
        string type = operation["type"].asString();

        // TODO: make a function map
        int num_examined2 = 0;
        int num_modified2 = 0;
        if (type == "body") {
            run_body(priority_scheduler, operation, controller, synapse_controller,
                    opt_rag, num_modified2, num_examined2); 
        } else if (type == "synapse") {
            run_synapse(priority_scheduler, operation, controller, synapse_controller,
                    opt_rag, num_modified2, num_examined2); 
        } else if (type == "edge") {
            run_edge(priority_scheduler, operation, controller, synapse_controller,
                    opt_rag, num_modified2, num_examined2); 
        } else if (type == "orphan") {
            run_orphan(priority_scheduler, operation, controller, synapse_controller,
                    opt_rag, num_modified2, num_examined2); 
        } else {
            throw ErrMsg("Unknow operation type: " + type);
        } 
        cout << endl;
         
        num_examined += num_examined2; 
        num_modified += num_modified2; 
        dump_differences(controller, synapse_controller, gt_controller, opt_rag, options);
    } 

    cout << "Total edges examined: " << num_examined << endl;
    cout << "Total edges modified: " << num_modified << endl;

}

void run_analyze_gt(AnalyzeGTOptions& options)
{
    //seg_filename, groundtruth_filename 
    VolumeLabelPtr seg_labels = VolumeLabelData::create_volume(
            options.seg_filename.c_str(), SEG_DATASET_NAME);
    cout << "Read seg stack" << endl;

    VolumeLabelPtr gt_labels = VolumeLabelData::create_volume(
            options.groundtruth_filename.c_str(), SEG_DATASET_NAME);
    cout << "Read GT stack" << endl;

    if (seg_labels->shape() != gt_labels->shape()) {
        throw ErrMsg("Mismatch in dimension sizes");
    }

    // create GT stack to get GT rag
    BioStack gt_stack(gt_labels);
    BioStackController gt_controller(&gt_stack);
    gt_controller.build_rag();
    cout << "Built GT RAG" << endl;

    // create seg stack
    BioStack stack(seg_labels);
    stack.set_gt_labelvol(gt_labels);
    BioStackController controller(&stack);
    controller.build_rag();
    cout << "Built seg RAG" << endl;
    controller.compute_groundtruth_assignment();

    if (options.graph_filename != "") {
        // TODO: rag utility that implements prob load from file
        // use previously generated probs
        RagPtr seg_rag_probs = RagPtr(create_rag_from_jsonfile(options.graph_filename.c_str())); 
        if (!seg_rag_probs) {
            throw ErrMsg("Problem processing graph file");
        }

        RagPtr seg_rag = stack.get_rag();
        for (Rag_uit::edges_iterator iter = seg_rag_probs->edges_begin();
                iter != seg_rag_probs->edges_end(); ++iter) {
            Label node1 = (*iter)->get_node1()->get_node_id();
            Label node2 = (*iter)->get_node2()->get_node_id();

            if (!(*iter)->is_false_edge()) {
                double prob = (*iter)->get_weight();
                RagEdge_uit* edge = seg_rag->find_rag_edge(node1, node2);
                edge->set_weight(prob);
            }
        }
    } else {
        // use optimal probs
        set_rag_probs(controller, stack.get_rag());
    }

    // create ground truth assignments to be maintained with rag
    RagPtr seg_rag = stack.get_rag();
    RagPtr rag_comp = RagPtr(new Rag_uit(*seg_rag));
    set_rag_probs(controller, rag_comp);


    // set synapse exclusions and create synapse stack for VI comparisons 
    VolumeLabelPtr tptr;
    Stack2 synapse_stack(tptr);
    if (options.synapse_filename != "") {
        controller.set_synapse_exclusions(options.synapse_filename.c_str());
        gt_controller.set_synapse_exclusions(options.synapse_filename.c_str());

        VolumeLabelPtr seg_syn_labels = controller.create_syn_label_volume();
        VolumeLabelPtr gt_syn_labels = gt_controller.create_syn_label_volume();
        synapse_stack.set_labelvol(seg_syn_labels);
        synapse_stack.set_gt_labelvol(gt_syn_labels);
    }
    StackController synapse_controller(&synapse_stack);

    // load body exclusions for comparative analysis (e.g., glia)
    if (options.exclusions_filename != "") {
        gt_controller.set_body_exclusions(options.exclusions_filename);
        stack.set_gt_labelvol(gt_stack.get_gt_labelvol());
    }

    // remove small bodies
    if (options.min_filter_size > 0) {
        unordered_set<Label_t> synapse_labels;
        controller.load_synapse_labels(synapse_labels);
        int num_removed = controller.remove_small_regions(options.min_filter_size,
                synapse_labels);
        cout << "Small seg bodies removed: " << num_removed << endl;

        unordered_set<Label_t> gt_synapse_labels;
        gt_controller.load_synapse_labels(gt_synapse_labels);
        num_removed = gt_controller.remove_small_regions(options.min_filter_size,
                gt_synapse_labels); 
        cout << "Small gt bodies removed: " << num_removed << endl;
    }

    // dilate edges for vi comparison
    if (options.gt_dilation > 0) {
        cout << "Create gt 0 boundaries" << endl;
        gt_controller.dilate_labelvol(options.gt_dilation);
        stack.set_gt_labelvol(gt_stack.get_labelvol());
    } 
    if (options.seg_dilation > 0) {
        controller.dilate_labelvol(options.seg_dilation);
        cout << "Created seg 0 boundaries" << endl;
    }        

    dump_differences(controller, synapse_controller, gt_controller, rag_comp, options);

    // find gt body errors
    int body_errors = num_body_errors(gt_controller, options.body_error_size);
    cout << "Number of GT orphan bodies with more than " << options.body_error_size << 
        " voxels : " << body_errors << endl;

    // find gt synapse errors
    if (options.synapse_filename != "") {
        int synapse_errors = num_synapse_errors(gt_controller, options.synapse_error_size); 
        cout << "Number of GT orphan bodies with more than " << options.synapse_error_size << 
            " synapse annotations : " << synapse_errors << endl;   
    }

    // try different strategies to refine the graph
    if (options.recipe_filename != "") {
        run_recipe(options.recipe_filename, controller, synapse_controller,
                gt_controller, rag_comp, options);
    }

    // dump final list of bad bodies if option is specified 
    if (options.dump_split_merge_bodies) {
        cout << "Showing body VI differences" << endl;
        dump_vi_differences(controller, options.vi_threshold);
        if (options.synapse_filename != "") {
            cout << "Showing synapse body VI differences" << endl;
            dump_vi_differences(synapse_controller, options.vi_threshold);
        }
    }
}


/*!
 * Main function for analyzing segmentation graph against ground truth
 * \param argc
 * \param argv
 * \return status
*/ 
int main(int argc, char** argv) 
{
    try {
        // default parameter values
        AnalyzeGTOptions options(argc, argv); 
        ScopeTime timer;

        run_analyze_gt(options);
    } catch (ErrMsg& err) {
        cerr << err.str << endl;
    }

    return 0;
}

