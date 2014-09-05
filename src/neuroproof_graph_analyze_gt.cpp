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

// feature manager
#include "../FeatureManager/FeatureMgr.h"

// for stack containing label volume and rag
#include "../BioPriors/BioStack.h"

// contains library for storing a region adjacency graph (RAG)
#include "../Rag/Rag.h"

// algorithms used to estimate how many edges need to be examine
#include "../EdgeEditor/EdgeEditor.h"

// simple function for measuring runtime
#include "../Utilities/ScopeTime.h"

// utilties for importing rags
#include "../Rag/RagIO.h"

// utitlies for parsing options
#include "../Utilities/OptionParser.h"

// for interacting with analyze service
#include <boost/network/protocol/http/client.hpp>

#include <libdvid/BinaryData.h>

#include "../Rag/RagUtils.h"

#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <map>

using std::multimap;
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
    graph_filename(""), callback_uri(""), exclusions_filename(""), recipe_filename(""),
    min_filter_size(0), random_seed(1) 
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
        parser.add_option(callback_uri, "callback-uri",
                "URI to post JSON results"); 
    
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
   
    //! uri to post json results
    string callback_uri;

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

/*!
 * Helper function for determining the number of segmentation
 * violations with respect the the annotated synapses.  Notably,
 * if a region/node with a synapse is orphan (i.e., does not have a
 * process that leaves the volume), that region/node is considered
 * an error.
 * \param stack segmentation stack with synapse information
 * \param threshold number of synapse annotations below which errors are ignored
 * \param total_synapse_errors gives the number of synapse annotations that were orphan
 * \return number of errors
*/
int num_synapse_errors(BioStack& stack, int threshold, int& total_synapse_errors)
{
    RagPtr rag = stack.get_rag();
    unordered_map<Label_t, int> synapse_counts;
    stack.load_synapse_counts(synapse_counts);
    int synapse_body_errors = 0;
    total_synapse_errors = 0;
    for (unordered_map<Label_t, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter) {
        if (iter->second >= threshold) {
            RagNode_t* node = rag->find_rag_node(iter->first);
            if (!(node->is_boundary())) {
                ++synapse_body_errors;
                total_synapse_errors += iter->second;
            }
        }
    }
    return synapse_body_errors;
}

double calc_voi_change(double size1, double size2,
        unsigned long long total_volume)
{
    double part1 = log(size1/total_volume)/log(2.0)*size1/total_volume + 
        log(size2/total_volume)/log(2.0)*size2/total_volume;
    double part2 = log((size1 + size2)/total_volume)/
                    log(2.0)*(size1 + size2)/total_volume;
    double part3 = -1 * (part1 - part2);
    return part3;
}

/*!
 * Helper function for determining the number of segmentation
 * violations with respect to the node/region size or morphology.
 * If a body above a certain threshold size of importance does not
 * leave the volume, it is considered an error.
 * \param stack segmentation stack
 * \param threshold node size below which errors are ignored
 * \return number of errors
*/
int num_body_errors(Stack& stack, int threshold)
{
    RagPtr rag = stack.get_rag();
    int body_errors = 0;
    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if ((!((*iter)->is_boundary())) && ((*iter)->get_size() >= threshold)) {
            ++body_errors;
        }
    }
    return body_errors;
}

/*!
 * Determine the set of labels (above the given thresholds for number
 * of member synapse annotations and node size) that do not leave the
 * image volume (i.e., orphans).
 * \param stack segmentation stack with synapse information
 * \param orphans set of orphans labels to be computed
 * \param threshold node size threshold
 * \param synapse_threshold number of synapse annotation threshold
*/ 
void load_orphans(BioStack& stack, unordered_set<Label_t>& orphans,
        int threshold, int synapse_threshold)
{
    unordered_map<Label_t, int> synapse_counts;
    stack.load_synapse_counts(synapse_counts);
    RagPtr rag = stack.get_rag();

    // find synapse orphans
    for (unordered_map<Label_t, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter) {
        if (iter->second >= synapse_threshold) {
            RagNode_t* node = rag->find_rag_node(iter->first);
            if (!(node->is_boundary())) {
                orphans.insert(iter->first);
            }
        }
    }

    // find big node orphans
    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if ((!((*iter)->is_boundary())) && ((*iter)->get_size() >= threshold)) {
            orphans.insert((*iter)->get_node_id());
        }
    }
}

/*!
 * Computes the variation of information (VI) metric between the segmentation
 * and ground truth and prints the body labels that contribute most to
 * errors in the segmentation.
 * \param stack segmentation stack
 * \param threshold VI threshold below which labels are not printed
*/
//void dump_vi_differences(Stack& stack, double threshold, RagPtr gt_rag)
void dump_vi_differences(Stack& stack, double threshold, Json::Value& json_data)
{
    double merge, split;
    multimap<double, Label_t> label_ranked;
    multimap<double, Label_t> gt_ranked;
    stack.compute_vi(merge, split, label_ranked, gt_ranked);

    cout << "Dumping VI differences to threshold: " << threshold << " where MergeSplit: ("
        << merge << ", " << split << ")" << endl;
    
    json_data["underseg-vi"] = merge;
    json_data["overseg-vi"] = split;

    double totalS = 0.0;
    double totalG = 0.0;
    // print undersegmented regions in the segmentation graph
    cout << endl << "******Seg Merged Bodies*****" << endl;
    unsigned int lnum = 0;
    for (multimap<double, Label_t>::reverse_iterator iter = label_ranked.rbegin();
            iter != label_ranked.rend(); ++iter) {
        if (merge < threshold) {
            break;
        }
        cout << iter->second << " " << iter->first << endl;
        merge -= iter->first;
        json_data["underseg-label"][lnum] = iter->second;
        json_data["underseg-val"][lnum] = iter->first;
        ++lnum;
    }
    cout << endl; 

    // print undersegmented region in the groundtruth graph
    // (oversegmentation in the segmentation compared to the groundtruth)
    cout << endl << "******GT Merged Bodies*****" << endl;
    lnum = 0;
    for (multimap<double, Label_t>::reverse_iterator iter = gt_ranked.rbegin();
            iter != gt_ranked.rend(); ++iter) {
        if (split < threshold) {
            //continue;
            break;
        }
        //if (gt_rag->find_rag_node(iter->second)->get_size() > 101500) {
            cout << iter->second << " " << iter->first << endl;
        //}
        split -= iter->first;
        json_data["overseg-label"][lnum] = iter->second;
        json_data["overseg-val"][lnum] = iter->first;
        ++lnum;
    }
    cout << endl; 
} 

/*!
 * Main function for computing the number of edits required to
 * improve the segmentation given a certain quality measure.  This
 * function uses the ground truth segmentation to simulate the actions
 * that a human annotator would do.
 * \param priority_scheduler algorithm for managing priority of edges
 * \param stack segmentation stack
 * \param synapse_stack segmentation stack of the synaptic annotations
 * \param opt_rag rag with the ground truth edge value
 * \param num_modified number of segmentation changes made
 * \param num_examined number of edges examined
 * \param use_exclusions not currently used
*/
void get_num_edits(EdgeEditor& priority_scheduler, Stack& stack,
        Stack& synapse_stack, RagPtr opt_rag, int& num_modified,
        int& num_examined, bool use_exclusions)
{
    int edges_examined = 0;
    int num_exclusions = 0;
    RagPtr rag = stack.get_rag();
    LowWeightCombine join_alg;
    double edge_size = 0.0;


    while (!priority_scheduler.isFinished()) {
        EdgeEditor::Location location;

        // choose most impactful edge given pre-determined strategy
        boost::tuple<Node_t, Node_t> pair = priority_scheduler.getTopEdge(location);
        
        Label_t node1 = boost::get<0>(pair);
        Label_t node2 = boost::get<1>(pair);
        RagEdge_t* temp_edge = opt_rag->find_rag_edge(node1, node2);
        edge_size += temp_edge->get_size();

        double weight = temp_edge->get_weight();
      
        bool merged = false;
        // use optimal prediction for decision
        if (weight < 0.00001) {
            ++num_modified;
            priority_scheduler.removeEdge(pair, true);
            // update rags and watersheds as appropriate
            rag_join_nodes(*opt_rag, opt_rag->find_rag_node(node1),
                    opt_rag->find_rag_node(node2), &join_alg); 
            stack.merge_labels(node2, node1, &join_alg, true);
            // no rag maintained for synapse stack
            if (synapse_stack.get_labelvol()) {
                synapse_stack.merge_labels(node2, node1, &join_alg, true);
            } 
            merged = true;
        } else {
            priority_scheduler.removeEdge(pair, false);
        }
        ++num_examined;
    }

    //cout << "Average edge size: " << edge_size / num_examined << endl;
}

/*!
 * Report the differences between both the segmentation stack and its
 * groundtruth and the segmentation  stack that only considers synapse
 * annotations compared and its groundtruth.  A series of different
 * comparisons are made (VI, number of violations, etc).
 * \param stack segmentation stack
 * \param synapse_stack segmentation stack of the synaptic annotations
 * \param gt_stack ground truth segmentation stack (TODO: wrap within stack) 
 * \param opt_rag rag with the ground truth edge value
 * \param options program options
*/
void dump_differences(BioStack& stack, Stack& synapse_stack,
        BioStack& gt_stack, RagPtr opt_rag, AnalyzeGTOptions& options, Json::Value& json_data)
{
    RagPtr seg_rag = stack.get_rag();
    RagPtr gt_rag = gt_stack.get_rag();

   
    cout << "************Printing Statistics**************" << endl; 
    cout << "Graph edges: " << seg_rag->get_num_edges() << endl;
    cout << "Graph nodes: " << seg_rag->get_num_regions() << endl;

    // vi computation
    cout << "Body VI: ";    
    double merge, split;
    stack.compute_vi(merge, split);
    cout << "MergeSplit: (" << merge << ", " << split << ")" << endl; 

    json_data["vol"]["underseg-vi"] = merge;
    json_data["vol"]["overseg-vi"] = split;

    // find body errors
    int body_errors = num_body_errors(stack, options.body_error_size);
    cout << "Number of orphan bodies with more than " << options.body_error_size << 
        " voxels : " << body_errors << endl;

    if (options.synapse_filename != "") {
        cout << "Synapse VI: ";
        synapse_stack.compute_vi(merge, split);
        cout << "MergeSplit: (" << merge << ", " << split << ")" << endl; 

        json_data["syn"]["underseg-vi"] = merge;
        json_data["syn"]["overseg-vi"] = split;
        // find synapse errors
        int total_orphan_synapse = 0; 
        int synapse_errors = num_synapse_errors(stack, options.synapse_error_size, total_orphan_synapse); 
        json_data["syn"]["orphans"] = total_orphan_synapse;
        cout << "Number of orphan bodies with at least " << options.synapse_error_size << 
            " synapse annotations : " << synapse_errors << "; total number of orphan synapses (Tbar or PSD): "
            << total_orphan_synapse << endl;   
    }
    
    unordered_set<Label_t> seg_orphans; 
    unordered_set<Label_t> gt_orphans; 
   
    // find percentage of orphans matched between segmentation and gt 
    load_orphans(stack, seg_orphans, options.body_error_size,
            options.synapse_error_size);
    load_orphans(gt_stack, gt_orphans, options.body_error_size,
            options.synapse_error_size);

    unordered_set<Label_t> seg_matched; 
    unordered_set<Label_t> gt_matched; 
    int matched = 0;
    for (std::tr1::unordered_set<Label_t>::iterator iter = seg_orphans.begin();
           iter != seg_orphans.end(); ++iter) {
        matched += stack.match_regions_overlap((*iter), gt_orphans,
                gt_rag, seg_matched, gt_matched);
    } 
    
    cout << "Percent orphans matched for seg: " <<
        seg_matched.size()/double(seg_orphans.size())*100 << endl;
    if (options.dump_orphans) {
        for (std::tr1::unordered_set<Label_t>::iterator iter = seg_orphans.begin();
                iter != seg_orphans.end(); ++iter) {
            if (seg_matched.find(*iter) == seg_matched.end()) {
                cout << "Unmatched seg orphan body: " << *iter << endl;
            } 
        }
    }
    cout << "Percent orphans matched for GT: " <<
        double(gt_matched.size())/gt_orphans.size()*100 << endl;
   
    // print orphans that are not found in gt 
    if (options.dump_orphans) {
        for (std::tr1::unordered_set<Label_t>::iterator iter = gt_orphans.begin();
                iter != gt_orphans.end(); ++iter) {
            if (gt_matched.find(*iter) == gt_matched.end()) {
                cout << "Unmatched GT orphan body: " << *iter << endl;
            } 
        }
    }
    int leftover = seg_orphans.size() - seg_matched.size() + gt_orphans.size() - gt_matched.size();
    cout << "Orphan agreement rate: " << double(matched)/(leftover + matched) * 100.0 <<  endl; 
     
    // might give weird results when dilation is done on GT ??
    stack.compute_groundtruth_assignment();
    vector<RagNode_t* > large_bodies;

    for (Rag_t::nodes_iterator iter = seg_rag->nodes_begin();
           iter != seg_rag->nodes_end(); ++iter) {
        if ((*iter)->get_size() > options.body_error_size) {
            large_bodies.push_back((*iter));
        }
    }

    // find pairs of large bodies that are not merged together but should be
    int num_unmerged = 0;
    Json::Value json_vals;
    //EdgeEditor<Label_t> temp_scheduler(*seg_rag, 0.1, 0.9, 0.1, json_vals);
    for (int i = 0; i < large_bodies.size(); ++i) {
        for (int j = (i+1); j < large_bodies.size(); ++j) {
            int val = stack.find_edge_label(large_bodies[i]->get_node_id(),
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
    

    std::tr1::unordered_map<Label_t, unsigned long long> body_changes;
    Rag_t opt_rag_copy(*opt_rag);

    bool change = true;
    LowWeightCombine join_alg;

    // optimally merge rag and keep track of biggest change
    while (change) {
        change = false;
        for (Rag_t::edges_iterator iter = opt_rag_copy.edges_begin();
                iter != opt_rag_copy.edges_end(); ++iter) {
            double weight = (*iter)->get_weight();
            if (weight < 0.0001) {
                RagNode_t* node1 = (*iter)->get_node1();
                RagNode_t* node2 = (*iter)->get_node2();

                unsigned long long large1 = node1->get_size();
                unsigned long long large2 = node2->get_size();
                if (body_changes.find(node1->get_node_id()) != body_changes.end()) {
                    large1 = body_changes[node1->get_node_id()];
                }
                if (body_changes.find(node2->get_node_id()) != body_changes.end()) {
                    large2 = body_changes[node2->get_node_id()];
                }
                RagNode_t* node_keep;
                RagNode_t* node_remove;
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

    // determine bodies whose total body size is much greater than its
    // largest original component
    int num_undermerged_bodies = 0;
    for (std::tr1::unordered_map<Label_t, unsigned long long>::iterator iter = body_changes.begin();
            iter != body_changes.end(); ++iter) {
        unsigned long long largest_orig = iter->second;
        RagNode_t* node1 = opt_rag_copy.find_rag_node(iter->first);
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

/*!
 * Run automatic editing algorithm using the size of nodes/bodies to
 * determine the editing that needs to performed to reach the desired threshold.
 * \param priority_scheduler algorithm for managing priority of edges
 * \param json_vals json values corresponding to a recipe for edting edges 
 * \param stack segmentation stack
 * \param synapse_stack segmentation stack of the synaptic annotations
 * \param opt_rag rag with the ground truth edge value
 * \param num_modified number of segmentation changes made
 * \param num_examined number of edges examined
*/
void run_body(EdgeEditor& priority_scheduler, Json::Value json_vals,
        Stack& stack, Stack& synapse_stack,
        RagPtr opt_rag, int& num_modified, int& num_examined)
{
    // path length 0 means no path limit
    unsigned int path_length = json_vals.get("path-length", 0).asUInt();
    // body size threshold for examining
    unsigned int threshold = json_vals.get("threshold", 25000).asUInt();
    double upper = json_vals.get("upper-prob", 0.99).asDouble();
    // not being used currently
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;

    cout << endl << "Starting body mode with threshold " << threshold << " and path length "
        << path_length << endl;

    priority_scheduler.set_body_mode(threshold, path_length, upper);

    get_num_edits(priority_scheduler, stack, synapse_stack, opt_rag,
            num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Body mode finished ... "; 
}

/*!
 * Run automatic editing algorithm using the number of synapse annotations to
 * determine the editing that needs to performed to reach the desired threshold.
 * \param priority_scheduler algorithm for managing priority of edges
 * \param json_vals json values corresponding to a recipe for edting edges 
 * \param stack segmentation stack
 * \param synapse_stack segmentation stack of the synaptic annotations
 * \param opt_rag rag with the ground truth edge value
 * \param num_modified number of segmentation changes made
 * \param num_examined number of edges examined
*/
void run_synapse(EdgeEditor& priority_scheduler, Json::Value json_vals,
        Stack& stack, Stack& synapse_stack,
        RagPtr opt_rag, int& num_modified, int& num_examined)
{
    // synapse connectivity threshold
    double threshold = json_vals.get("threshold", 0.1).asDouble();
    double upper = json_vals.get("upper-prob", 0.99).asDouble();
    // not being used currently
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;

    cout << endl << "Starting synapse mode with threshold " << threshold << endl;

    priority_scheduler.set_synapse_mode(threshold, upper);

    get_num_edits(priority_scheduler, stack, synapse_stack, opt_rag,
            num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Synapse mode finished ... "; 
}

/*!
 * Run automatic editing algorithm using the orphan status of a node to
 * determine the editing that needs to performed to reach the desired threshold.
 * \param priority_scheduler algorithm for managing priority of edges
 * \param json_vals json values corresponding to a recipe for edting edges 
 * \param stack segmentation stack
 * \param synapse_stack segmentation stack of the synaptic annotations
 * \param opt_rag rag with the ground truth edge value
 * \param num_modified number of segmentation changes made
 * \param num_examined number of edges examined
*/
void run_orphan(EdgeEditor& priority_scheduler, Json::Value json_vals,
        Stack& stack, Stack& synapse_stack,
        RagPtr opt_rag, int& num_modified, int& num_examined)
{
    // orphan size threshold
    unsigned int threshold = json_vals.get("threshold", 25000).asUInt();
    // exclusions not being used currently
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;
    cout << endl << "Starting orphan mode with threshold " << threshold << endl;

    priority_scheduler.set_orphan_mode(threshold);

    get_num_edits(priority_scheduler, stack, synapse_stack, opt_rag,
            num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Orphan mode finished ... "; 
}

/*!
 * Run automatic editing algorithm using the uncertainty between edges to 
 * determine the editing that needs to performed to reach the desired threshold.
 * \param priority_scheduler algorithm for managing priority of edges
 * \param json_vals json values corresponding to a recipe for edting edges 
 * \param stack segmentation stack
 * \param synapse_stack segmentation stack of the synaptic annotations
 * \param opt_rag rag with the ground truth edge value
 * \param num_modified number of segmentation changes made
 * \param num_examined number of edges examined
*/
void run_edge(EdgeEditor& priority_scheduler, Json::Value json_vals,
        Stack& stack, Stack& synapse_stack,
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

    get_num_edits(priority_scheduler, stack, synapse_stack, opt_rag,
            num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Edge mode finished ... "; 
}

/*!
 * Determine the edge weight values of a RAG by examining
 * the decision label derived in a stack containing groundtruth.
 * \param segmentation segmentation stack
 * \param rag point to RAG
*/
void set_rag_probs(Stack& stack, RagPtr rag) {
    for (Rag_t::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        int val = stack.find_edge_label((*iter)->get_node1()->get_node_id(),
                (*iter)->get_node2()->get_node_id()); 
        if (val == -1) {
            (*iter)->set_weight(0.0);
        } else {
            (*iter)->set_weight(1.0);
        }
    }
}

void print_uncertainty_distribution(Stack& stack, RagPtr rag)
{
    int precision = 100;
    vector<int> merge_count(precision+1, 0);
    vector<int> split_count(precision+1, 0);

    for (Rag_t::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        int val = stack.find_edge_label((*iter)->get_node1()->get_node_id(),
                (*iter)->get_node2()->get_node_id()); 
        double weight = (*iter)->get_weight();
        int bin = int(weight * precision + 0.5) % (precision+1);

        if (val == -1) {
            merge_count[bin] += 1; 
        } else {
            split_count[bin] += 1; 
        }

    }

    // dump bins
    for (int i = 0; i <= precision; ++i) {
        cout << "Bin: " << i << ", merge: " << merge_count[i] << ", split: " << split_count[i] << endl;
    }

    // dump cumulative distribution
    int total_split = 0;
    double total_ideal = 0;
    for (int i = 0; i <= precision; ++i) {
        total_split += split_count[i];

        // find how many in the bucket
        int temp_total = split_count[i] + merge_count[i]; 
        total_ideal += (temp_total * double(i)/precision);
        
        cout << "Prob: " << i << ", " << total_split << ", " << total_ideal << endl;
    }

}



/*!
 * Runs a recipe created in a json file that specifies which types
 * of simulated manual editing should be performed.  The JSON file format
 * is '{"recipe" : []}' with a list of different editing actions
 * that should be performed to some threshold.  For instance,
 * '{ "type" : "body", "path-length" : 0, "threshold" : 10000}' could
 * be added to the list and would specify that the algorithm should
 * consider all edges that are on a path (with no path length limit)
 * where the uncertainty of that path suggests an information change
 * more significant than 10,000 voxels.
 * \param recipe_filename json file with editing operations to be performed
 * \param stack segmentation stack with synapse information
 * \param synapse_stack segmentation stack of the synaptic annotations
 * \param gt_stack ground truth stack with synapse information
 * \param opt_rag rag with the ground truth edge value
 * \param options program options
*/
void run_recipe(string recipe_filename, BioStack& stack,
        Stack& synapse_stack, BioStack& gt_stack,
        RagPtr opt_rag, AnalyzeGTOptions& options, Json::Value& json_data)
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

    RagPtr rag = stack.get_rag();

    Json::Value json_vals_priority;
    
    // load orphan information
    Json::Value orphan_bodies;
    unsigned int id = 0;
    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if (!((*iter)->is_boundary())) {
            orphan_bodies[id++] = (*iter)->get_node_id(); 
        }
    }
    
    // load default information
    for (Rag_t::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        (*iter)->set_property("location", Location(0,0,0));
        (*iter)->set_property("edge_size", (*iter)->get_size());
    }
    
    json_vals_priority["orphan_bodies"] = orphan_bodies;

    // load synapse information
    unordered_map<Label_t, int> synapse_counts;
    stack.load_synapse_counts(synapse_counts);
    id = 0;
    Json::Value synapses;
    for (std::tr1::unordered_map<Label_t, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter) {
        Json::Value val_pair;
        val_pair[(unsigned int)(0)] = iter->first;
        val_pair[(unsigned int)(1)] = iter->second;
        synapses[id++] = val_pair;
    }
    json_vals_priority["synapse_bodies"] = synapses;

    // create priority algorithm using json data computed
    // TODO: enable initialization without json
    EdgeEditor priority_scheduler(*rag, 0.1, 0.9, 0.1, json_vals_priority);
    
    int num_examined = 0;
    int num_modified = 0;

    // try all the editing operations in the recipe list
    // the same type can be called multiple times
    for (int i = 0; i < json_vals["recipe"].size(); ++i) {
        Json::Value operation = json_vals["recipe"][i]; 
        string type = operation["type"].asString();

        // TODO: make a function map
        int num_examined2 = 0;
        int num_modified2 = 0;
        if (type == "body") {
            run_body(priority_scheduler, operation, stack, synapse_stack,
                    opt_rag, num_modified2, num_examined2);
            json_data["vol"]["edges-examined"] = num_examined2; 
        } else if (type == "synapse") {
            run_synapse(priority_scheduler, operation, stack, synapse_stack,
                    opt_rag, num_modified2, num_examined2); 
            json_data["syn"]["edges-examined"] = num_examined2; 
        } else if (type == "edge") {
            run_edge(priority_scheduler, operation, stack, synapse_stack,
                    opt_rag, num_modified2, num_examined2); 
            json_data["edge"]["edges-examined"] = num_examined2; 
        } else if (type == "orphan") {
            run_orphan(priority_scheduler, operation, stack, synapse_stack,
                    opt_rag, num_modified2, num_examined2); 
            json_data["orphan"]["edges-examined"] = num_examined2; 
        } else {
            throw ErrMsg("Unknow operation type: " + type);
        } 
        cout << endl;
         
        num_examined += num_examined2; 
        num_modified += num_modified2; 
    
        Json::Value current_status;
        dump_differences(stack, synapse_stack, gt_stack, opt_rag, options, current_status);
    } 

    cout << "Total edges examined: " << num_examined << endl;
    cout << "Total edges modified: " << num_modified << endl;

}

void print_synapse_violations(const char* synapse_json, VolumeLabelPtr labels, VolumeLabelPtr gtlabels)
{
    unsigned int ysize = labels->shape(1);
    Json::Reader json_reader;
    Json::Value json_reader_vals;
    
    ifstream fin(synapse_json);
    if (!fin) {
        throw ErrMsg("Error: input file: " + string(synapse_json) + " cannot be opened");
    }
    if (!json_reader.parse(fin, json_reader_vals)) {
        throw ErrMsg("Error: Json incorrectly formatted");
    }
    fin.close();
 
    Json::Value synapses = json_reader_vals["data"];

    // violations are false mergers; false violations is a merge that the seg wouldn't do
    int total_violations = 0;
    int true_violations = 0;
    int total_tsynapses = 0;
    int total_tvio = 0;
    int total_pairs = 0;
    int total_gtmerges = 0;

    for (int i = 0; i < synapses.size(); ++i) {
        vector<vector<unsigned int> > locations;
        Json::Value location = synapses[i]["T-bar"]["location"];
        ++total_tsynapses;
        if (!location.empty()) {
            vector<unsigned int> loc;
            loc.push_back(location[(unsigned int)(0)].asUInt());
            loc.push_back(ysize - location[(unsigned int)(1)].asUInt() - 1);
            loc.push_back(location[(unsigned int)(2)].asUInt());
            locations.push_back(loc);
        }
        Json::Value psds = synapses[i]["partners"];
        for (int i = 0; i < psds.size(); ++i) {
            Json::Value location = psds[i]["location"];
            if (!location.empty()) {
                vector<unsigned int> loc;
                loc.push_back(location[(unsigned int)(0)].asUInt());
                loc.push_back(ysize - location[(unsigned int)(1)].asUInt() - 1);
                loc.push_back(location[(unsigned int)(2)].asUInt());
                locations.push_back(loc);
            }
        }

        bool violation = false;
        for (int iter1 = 0; iter1 < locations.size(); ++iter1) {
            ++total_pairs;
            for (int iter2 = (iter1 + 1); iter2 < locations.size(); ++iter2) {
                
                Label_t body1 = (*labels)(locations[iter1][0], locations[iter1][1], locations[iter1][2]);
                Label_t gtbody1 = (*gtlabels)(locations[iter1][0], locations[iter1][1], locations[iter1][2]);
                Label_t body2 = (*labels)(locations[iter2][0], locations[iter2][1], locations[iter2][2]);
                Label_t gtbody2 = (*gtlabels)(locations[iter2][0], locations[iter2][1], locations[iter2][2]);
                if (body1 == body2) {
                    ++total_violations;
                    if (gtbody1 != gtbody2) {
                        violation = true;
                        cout << "Vio: " << body1 << endl;
                        ++true_violations;
                    }
                }
                if (gtbody1 == gtbody2) {
                    ++total_gtmerges;
                }

            }
        }
        if (violation) {
            ++total_tvio;
        }
    }

    cout << "Total constraint violations: " << total_violations << endl;
    cout << "Total true violations: " << true_violations << endl;
    cout << "Total gt false violations: " << total_gtmerges << endl;
    cout << "Total possible constraints: " << total_pairs << endl;
    cout << "Total tbars: " << total_tsynapses << endl;
    cout << "Total tbars with true violation: " << total_tvio << endl;
}

void load_json(string uri, Json::Value& json_data, boost::network::http::client& request_client)
{
    if (uri == "") {
        return; 
    }
    boost::network::http::client::request requestobj(uri);
    requestobj << boost::network::header("Connection", "close");

    stringstream datastr;
    datastr << json_data;
    libdvid::BinaryDataPtr value = 
        libdvid::BinaryData::create_binary_data(datastr.str().c_str(), datastr.str().length());
    boost::network::http::client::response respdata = request_client.post(requestobj,
            value->get_data(), string("application/octet-stream"));
    int status_code = status(respdata);
    if (status_code != 200) {
        throw ErrMsg(body(respdata));
    }
}

/*!
 * Main function that calls into the various functions that
 * analyze the segmentation with respect to the ground truth.
 * \param options program options
*/
void run_analyze_gt(AnalyzeGTOptions& options)
{
    clock_t initial_time = clock();
    boost::network::http::client request_client;
    Json::Value status_json;
    status_json["status"] = "Loading label volumes";
    load_json(options.callback_uri, status_json, request_client);
    
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
    
    status_json["status"] = "Analyzing similarities";
    load_json(options.callback_uri, status_json, request_client);

    // create GT stack to get GT rag
    BioStack gt_stack(gt_labels);
    gt_stack.Stack::build_rag();
    cout << "Built GT RAG" << endl;

    // create seg stack
    BioStack stack(seg_labels);
    stack.set_gt_labelvol(gt_labels);
    stack.Stack::build_rag();
    cout << "Built seg RAG" << endl;
    stack.compute_groundtruth_assignment();

    // evaluate synapse violations
    //print_synapse_violations(options.synapse_filename.c_str(), seg_labels, gt_labels);

    if (options.graph_filename != "") {
        // TODO: rag utility that implements prob load from file
        // use previously generated probs
        RagPtr seg_rag_probs = RagPtr(create_rag_from_jsonfile(options.graph_filename.c_str())); 
        if (!seg_rag_probs) {
            throw ErrMsg("Problem processing graph file");
        }

        RagPtr seg_rag = stack.get_rag();
        for (Rag_t::edges_iterator iter = seg_rag_probs->edges_begin();
                iter != seg_rag_probs->edges_end(); ++iter) {
            Node_t node1 = (*iter)->get_node1()->get_node_id();
            Node_t node2 = (*iter)->get_node2()->get_node_id();

            if (!(*iter)->is_false_edge()) {
                double prob = (*iter)->get_weight();
                RagEdge_t* edge = seg_rag->find_rag_edge(node1, node2);
                edge->set_weight(prob);
            }
        }

        /*
        RagPtr seg_rag2 = stack.get_rag();
        for (Rag_t::edges_iterator iter = seg_rag2->edges_begin();
                iter != seg_rag2->edges_end(); ++iter) {
            int weight = rand()%101;
            (*iter)->set_weight(weight/100.0);
        }*/

        // dump probs
        //print_uncertainty_distribution(stack, stack.get_rag());
    } else {
        // use optimal probs
        set_rag_probs(stack, stack.get_rag());
    }

#if 0
    RagPtr gt_rag2 = gt_stack.get_rag();
    for (Rag_t::nodes_iterator iter = gt_rag2->nodes_begin();
            iter != gt_rag2->nodes_end(); ++iter) {
        cout  << "GT size: " << (*iter)->get_size() << endl;
    }
    exit(1);
#endif

#if 0
    // ------------- check how reasonable the segmentation is ----------------------
    cout << "Checking segmentation reasonableness..." << endl;
    RagPtr gt_rag = gt_stack.get_rag();
    unordered_map<Label_t, vector<Label_t> > gt2segs;
    stack.get_gt2segs_map(gt_rag, gt2segs);
    int threshold2 = 55000; // 55756 gives us 90 percent coverage on the gt side
    int threshold3 = 105000; // 105467 gives us 90 percent coverage on the gt side (minimize border bodies)
    int threshold = 25000;
    int num_big_bodies = 0;
    
    unordered_map<Label_t, int> max_depth;
    unordered_map<Label_t, double> max_prob_depth;
    RagPtr seg_rag2 = stack.get_rag();

   
    /* 
    unsigned long long volume_size = gt_rag->get_rag_size();
    vector<unsigned long long> sizes;
    for (Rag_t::nodes_iterator iter = gt_rag->nodes_begin();
            iter != gt_rag->nodes_end(); ++iter) {
        sizes.push_back((*iter)->get_size());
    }
    sort(sizes.begin(), sizes.end());
    unsigned long long accum = 0;
    unsigned long long accum_thres = (unsigned long long)(volume_size * 0.90);
    for (int i = sizes.size()-1; i>=0; --i) {
        accum += sizes[i];
        if (accum > accum_thres) {
            cout << "GT size threshold: " << sizes[i] << endl;
            break;
        }
    }
    cout << "Total size: " << volume_size << endl;
    */
    unordered_map<Label_t, unsigned long long> gt_count;
    unsigned long long volume_size = 0; 
    
    volume_forXYZ(*gt_labels, x, y, z) {
        Label_t label = (*gt_labels)(x,y,z); 
        if (!label) {
            continue;
        }
        if (x < 150 || y < 150 || z < 150) {
            continue;
        }
        if (x > 350 || y > 350 || z > 350) {
            continue;
        }
        ++volume_size;
        gt_count[label]++;
    }
    unsigned long long accum_thres = (unsigned long long)(volume_size * 0.90);
    unsigned long long accum = 0;

    multimap<unsigned long long, Label_t> sizes;
    for (unordered_map<Label_t, unsigned long long>::iterator iter = gt_count.begin();
            iter != gt_count.end(); ++iter) {
        sizes.insert(std::make_pair(gt_rag->find_rag_node(iter->first)->get_size(), iter->first));
    }

    for (multimap<unsigned long long, Label_t>::reverse_iterator iter = sizes.rbegin();
            iter != sizes.rend(); ++iter) {
        accum += gt_count[iter->second];
        if (accum > accum_thres) {
            cout << "GT size threshold: " << iter->first << endl;
            break;
        }
    }



    for (int m = 1000; m <= 25000; m+=1000) {
    int num_bad_bodies = 0;
    num_big_bodies = 0;
    threshold = m;
    

    
    for (unordered_map<Label_t, vector<Label_t> >::iterator iter = gt2segs.begin();
            iter != gt2segs.end(); ++iter) {
        if (gt_rag->find_rag_node(iter->first)->get_size() < 105000) {
            continue;
        }
        ++num_big_bodies;
        vector<Label_t>& segs = iter->second;
        unsigned long long total_good = 0;
        unsigned long long total_bad = 0;
        for (int i = 0; i < segs.size(); ++i) {
            if (seg_rag2->find_rag_node(segs[i])->get_size() > threshold) {
                total_good += seg_rag2->find_rag_node(segs[i])->get_size();
            } else {
                total_bad += seg_rag2->find_rag_node(segs[i])->get_size();
            }
        }        
        //cout << segs.size() << " " << total_good/double(total_good+total_bad)*100 << endl;

        if (total_bad > 25000) {
            ++num_bad_bodies;
        }

        /*
        if (segs.size() == 1) {
            max_prob_depth[iter->first] = 1;
            max_depth[iter->first] = 0;
            continue;
        }
    
        // iterate all segs and get maximum depth and minimum prob    
        max_prob_depth[iter->first] = 0;
        for (int i = 0; i < segs.size(); ++i) {
            for (int j = i+1; j < segs.size(); ++j) {
                double prob = find_affinity_path(*seg_rag2, seg_rag2->find_rag_node(segs[i]),
                        seg_rag2->find_rag_node(segs[j]));

                //if (prob < max_prob_depth[iter->first]) {
                //    max_prob_depth[iter->first] += (prob/((segs.size()-1)*(segs.size())/2));
                //}
                
                //if (prob > max_prob_depth[iter->first]) {
                    max_prob_depth[iter->first] += (prob/((segs.size()-1)*(segs.size())/2));
                //}

            }
        }*/

        //cout << segs.size() << " " << max_prob_depth[iter->first] << endl;
    }
    cout << "Number of bad bodies: " << num_bad_bodies << endl;
    cout << "Percentage of bad bodies: " << num_bad_bodies/double(num_big_bodies)*100 << ", threshold: " << m << endl;
    }

    cout << "Number of big bodies: " << num_big_bodies << endl;
    /*int precision = 100;
    vector<int> prob_count(precision+1, 0);
    for (unordered_map<Label_t, double>::iterator iter = max_prob_depth.begin();
           iter != max_prob_depth.end(); ++iter) {
        prob_count[int(iter->second * precision + 0.5) % precision+1] += 1;
    } 
    for (int i = 0; i <= precision; ++i) {
        cout << "Prob connected: " << i << " " << prob_count[i] << endl;
    }*/

    // -------------  end check how reasonable the segmentation is ----------------------
#endif

    // create ground truth assignments to be maintained with rag
    RagPtr seg_rag = stack.get_rag();
    RagPtr rag_comp = RagPtr(new Rag_t(*seg_rag));
    set_rag_probs(stack, rag_comp);


    // set synapse exclusions and create synapse stack for VI comparisons 
    VolumeLabelPtr tptr;
    Stack synapse_stack(tptr);
    if (options.synapse_filename != "") {
        stack.set_synapse_exclusions(options.synapse_filename.c_str());
        gt_stack.set_synapse_exclusions(options.synapse_filename.c_str());

        VolumeLabelPtr seg_syn_labels = stack.create_syn_label_volume();
        VolumeLabelPtr gt_syn_labels = gt_stack.create_syn_label_volume();
        synapse_stack.set_labelvol(seg_syn_labels);
        synapse_stack.set_gt_labelvol(gt_syn_labels);
    }

    // load body exclusions for comparative analysis (e.g., glia)
    if (options.exclusions_filename != "") {
        gt_stack.set_body_exclusions(options.exclusions_filename);
        stack.set_gt_labelvol(gt_stack.get_gt_labelvol());
    }

    // remove small bodies
    if (options.min_filter_size > 0) {
        unordered_set<Label_t> synapse_labels;
        stack.load_synapse_labels(synapse_labels);
        int num_removed = stack.remove_small_regions(options.min_filter_size,
                synapse_labels);
        cout << "Small seg bodies removed: " << num_removed << endl;

        unordered_set<Label_t> gt_synapse_labels;
        gt_stack.load_synapse_labels(gt_synapse_labels);
        num_removed = gt_stack.remove_small_regions(options.min_filter_size,
                gt_synapse_labels); 
        cout << "Small gt bodies removed: " << num_removed << endl;
    }

    // dilate edges for vi comparison
    if (options.gt_dilation > 0) {
        cout << "Create gt 0 boundaries" << endl;
        gt_stack.dilate_labelvol(options.gt_dilation);
        stack.set_gt_labelvol(gt_stack.get_labelvol());
    } 
    if (options.seg_dilation > 0) {
        stack.dilate_labelvol(options.seg_dilation);
        cout << "Created seg 0 boundaries" << endl;
    }        

    Json::Value initial_status;
    dump_differences(stack, synapse_stack, gt_stack, rag_comp, options, initial_status);
    status_json["start"] = initial_status;

    // find gt body errors
    int body_errors = num_body_errors(gt_stack, options.body_error_size);
    cout << "Number of GT orphan bodies with more than " << options.body_error_size << 
        " voxels : " << body_errors << endl;

    // find gt synapse errors
    if (options.synapse_filename != "") {
        int total_orphan_synapse = 0; 
        int synapse_errors = num_synapse_errors(gt_stack, options.synapse_error_size, total_orphan_synapse); 
        cout << "Number of GT orphan bodies with at least " << options.synapse_error_size << 
            " synapse annotations : " << synapse_errors << "; total number of orphan synapses (Tbar or PSD): "
            << total_orphan_synapse << endl;   
    }
    
    status_json["status"] = "Performing automatic proofreading";
    load_json(options.callback_uri, status_json, request_client);

    // try different strategies to refine the graph
    if (options.recipe_filename != "") {
        Json::Value recipe_data;
        run_recipe(options.recipe_filename, stack, synapse_stack,
                gt_stack, rag_comp, options, recipe_data);
        status_json["proofreading-work"] = recipe_data;
    }

    // biologically important change
//    double voi_change_thres = calc_voi_change(25000, 101500, seg_rag->get_rag_size());
//    RagPtr gt_rag = gt_stack.get_rag();

    // dump final list of bad bodies if option is specified 
    if (options.dump_split_merge_bodies) {
        cout << "Showing body VI differences" << endl;
        Json::Value vol_json;
        dump_vi_differences(stack, options.vi_threshold, vol_json);
        status_json["final"]["vol"] = vol_json;
        //dump_vi_differences(stack, voi_change_thres, gt_rag);
        if (options.synapse_filename != "") {
            cout << "Showing synapse body VI differences" << endl;
            Json::Value syn_json;
            dump_vi_differences(synapse_stack, options.vi_threshold, syn_json);
            status_json["final"]["syn"] = syn_json;
           
#if 0 
            unsigned long long volume_size = 0;
            // designate the number of synapse annotations as being a node's size
            for (Rag_t::nodes_iterator iter = seg_rag->nodes_begin();
                    iter != seg_rag->nodes_end(); ++iter) {
                unsigned long long synapse_weight = 0;
                try {   
                    synapse_weight = (*iter)->get_property<unsigned long long>("synapse_weight");
                } catch(...) {
                    //
                }
                if (synapse_weight == 0) {
                    continue;
                }

                volume_size += synapse_weight;
            }
            double voi_change_thres = calc_voi_change(0.1, 0.1, volume_size);
            dump_vi_differences(synapse_stack, voi_change_thres, gt_rag);
#endif
        }
    }

    status_json["status"] = "Finished";
    status_json["runtime"] = (clock() - initial_time) / double(CLOCKS_PER_SEC);
    load_json(options.callback_uri, status_json, request_client);
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

