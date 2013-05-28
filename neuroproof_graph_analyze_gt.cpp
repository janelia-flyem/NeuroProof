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

// holder for label volume
#include "DataStructures/Stack.h" 

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
    random_seed(1), enable_transforms(true)
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
        parser.add_option(enable_transforms, "transforms",
                "enables using the transforms table when reading the segmentation", true, false, true); 

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
    bool enable_transforms;
};

template <class T>
void padZero(T* data, const size_t* dims, int pad_length, T** ppadded_data){

    // implemented only for 3D arrays

    unsigned long int newsize = (dims[0]+2*pad_length)*(dims[1]+2*pad_length)*(dims[2]+2*pad_length);	
    *ppadded_data = new T[newsize];  	
    T* padded_data = *ppadded_data;

    memset((void*) padded_data, 0, sizeof(T)*newsize);

    unsigned int width, plane_size, width0, plane_size0, i0,j0,k0, i,j,k;

    for (i=pad_length, i0=0; i<= dims[0] ; i++, i0++)
        for (j=pad_length, j0=0; j<= dims[1]; j++, j0++)	
            for(k=pad_length, k0=0; k<= dims[2]; k++, k0++){
                plane_size = (dims[1]+2*pad_length)*(dims[2]+2*pad_length);	
                width = dims[2]+2*pad_length;	

                plane_size0 = dims[1]*dims[2];	
                width0 = dims[2];	

                padded_data[i*plane_size+ j*width + k] = data[i0*plane_size0 + j0*width0 + k0];	
            }
}


Label* get_label_volume(string label_filename, bool enable_transforms, int& depth,
        int& height, int& width)
{
    unordered_map<Label, Label> sp2body;
    if (enable_transforms) {
        // read transforms from watershed/segmentation file
        H5Read transforms(label_filename.c_str(), "transforms");
        unsigned long long* transform_data=0;	
        transforms.readData(&transform_data);	
        int transform_height = transforms.dim()[0];
        int transform_width = transforms.dim()[1];

        // create sp to body map
        int tpos = 0;
        sp2body[0] = 0;
        for (int i = 0; i < transform_height; ++i, tpos+=2) {
            sp2body[(transform_data[tpos])] = transform_data[tpos+1];
        }
        delete[] transform_data;
    }

    H5Read watershed(label_filename.c_str(),SEG_DATASET_NAME);	
    depth =	 watershed.dim()[0];
    height = watershed.dim()[1];
    width =	 watershed.dim()[2];
    size_t dims[3];
    dims[0] = depth;
    dims[1] = height;
    dims[2] = width;

    Label* watershed_data=0;	
    if (watershed.get_size() == sizeof(unsigned long long)) {
        unsigned long long * temp_data = 0;
        watershed.readData(&temp_data);
        unsigned long long total_size = depth*height*width;
        watershed_data = new Label[total_size];
        for (unsigned long long i = 0; i < total_size; ++i) {
            watershed_data[i] = temp_data[i];
        }
        delete [] temp_data;
    } else {
        watershed.readData(&watershed_data);
    }        
    
    // map supervoxel ids to body ids
    unsigned long int total_size = depth * height * width;
    if (enable_transforms) {
        for (unsigned long int i = 0; i < total_size; ++i) {
            watershed_data[i] = sp2body[(watershed_data[i])];
        }
    }

    Label *zp_watershed_data=0;
    padZero(watershed_data, dims,PADDING,&zp_watershed_data);	
    delete[] watershed_data;

    return zp_watershed_data; 
}

void get_num_edits(LocalEdgePriority<Label>& priority_scheduler, Stack* seg_stack,
        Stack* synapse_stack, Rag<Label>* opt_rag, int& num_modified,
        int& num_examined, bool use_exclusions)
{
    int edges_examined = 0;
    int num_exclusions = 0;
    Rag<Label>* rag = seg_stack->get_rag();

    while (!priority_scheduler.isFinished()) {
        EdgePriority<Label>::Location location;

        // choose most impactful edge given pre-determined strategy
        boost::tuple<Label, Label> pair = priority_scheduler.getTopEdge(location);
        
        Label node1 = boost::get<0>(pair);
        Label node2 = boost::get<1>(pair);
        RagEdge<Label>* temp_edge = opt_rag->find_rag_edge(node1, node2);
        double weight = temp_edge->get_weight();

        bool merged = false;
        // use optimal prediction for decision
        if (weight < 0.00001) {
            ++num_modified;
            priority_scheduler.removeEdge(pair, true);
            // update rags and watersheds as appropriate
            vector<string> property_names;
            rag_merge_edge(*opt_rag, temp_edge, opt_rag->find_rag_node(node1), property_names); 
            seg_stack->merge_nodes(node1, node2);
            // no rag maintained for synapse stack
            if (synapse_stack) {
                synapse_stack->merge_nodes(node1, node2);
            } 
            merged = true;
        } else {
            priority_scheduler.removeEdge(pair, false);
        }
       
        // set body exclusions if they exist
        bool exclusion_found = false;
        if (use_exclusions) {
            if (!merged && (seg_stack->is_excluded(node2))) {
                ++num_exclusions;
                exclusion_found = true;
                RagNode<Label>* rnode2 = rag->find_rag_node(node2);
                for (RagNode<Label>::edge_iterator iter = rnode2->edge_begin();
                        iter != rnode2->edge_end(); ++iter) {
                    (*iter)->set_preserve(true);
                }
            }
            if (seg_stack->is_excluded(node1)) {
                ++num_exclusions;
                exclusion_found = true;
                RagNode<Label>* rnode1 = rag->find_rag_node(node1);
                for (RagNode<Label>::edge_iterator iter = rnode1->edge_begin();
                        iter != rnode1->edge_end(); ++iter) {
                    (*iter)->set_preserve(true);
                }
            }
            if (exclusion_found) {
                priority_scheduler.updatePriority();
            }
        }
        
        ++num_examined;
    }
    
    if (use_exclusions) {
        cout << "Number of body exclusions set: " << num_exclusions << endl;
    }
}


void load_orphans(Stack* stack, Rag<Label>* rag, std::tr1::unordered_set<Label>& orphans,
        int threshold, int synapse_threshold)
{
    std::tr1::unordered_map<Label, int> synapse_counts;
    stack->load_synapse_counts(synapse_counts);
    for (std::tr1::unordered_map<Label, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter) {
        if (iter->second >= synapse_threshold) {
            RagNode<Label>* node = rag->find_rag_node(iter->first);
            if (!(node->is_border())) {
                orphans.insert(iter->first);
            }
        }
    }

    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if ((!((*iter)->is_border())) && ((*iter)->get_size() >= threshold)) {
            orphans.insert((*iter)->get_node_id());
        }
    }
}


int num_synapse_errors(Stack* stack, Rag<Label>* rag, int threshold)
{
    std::tr1::unordered_map<Label, int> synapse_counts;
    stack->load_synapse_counts(synapse_counts);
    int synapse_errors = 0;
    for (std::tr1::unordered_map<Label, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter) {
        if (iter->second >= threshold) {
            RagNode<Label>* node = rag->find_rag_node(iter->first);
            if (!(node->is_border())) {
                ++synapse_errors;
            }
        }
    }
    return synapse_errors;
}

int num_body_errors(Rag<Label>* rag, int threshold)
{
    int body_errors = 0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if ((!((*iter)->is_border())) && ((*iter)->get_size() >= threshold)) {
            ++body_errors;
        }
    }
    return body_errors;
}

void dump_differences(Stack* seg_stack, Stack* synapse_stack, Stack * gt_stack, 
        Rag<Label>* opt_rag, AnalyzeGTOptions& options)
{
    Rag<Label>* seg_rag = seg_stack->get_rag();
    Rag<Label>* gt_rag = gt_stack->get_rag();
   
    cout << "************Printing Statistics**************" << endl; 
    cout << "Graph edges: " << seg_rag->get_num_edges() << endl;
    cout << "Graph nodes: " << seg_rag->get_num_regions() << endl;

    cout << "Body VI: ";    
    seg_stack->compute_vi();

    // find body errors
    int body_errors = num_body_errors(seg_rag, options.body_error_size);
    cout << "Number of orphan bodies with more than " << options.body_error_size << 
        " voxels : " << body_errors << endl;

    if (synapse_stack) {
        cout << "Synapse VI: ";
        synapse_stack->compute_vi();
       
        // find synapse errors 
        int synapse_errors = num_synapse_errors(seg_stack, seg_rag, options.synapse_error_size); 
        cout << "Number of orphan bodies with more than " << options.synapse_error_size << 
            " synapse annotations : " << synapse_errors << endl;   
    }

    
    std::tr1::unordered_set<Label> seg_orphans; 
    std::tr1::unordered_set<Label> gt_orphans; 
    
    load_orphans(seg_stack, seg_rag, seg_orphans, options.body_error_size,
            options.synapse_error_size);
    load_orphans(gt_stack, gt_rag, gt_orphans, options.body_error_size,
            options.synapse_error_size);

    std::tr1::unordered_set<Label> seg_matched; 
    std::tr1::unordered_set<Label> gt_matched; 
    int matched = 0;
    for (std::tr1::unordered_set<Label>::iterator iter = seg_orphans.begin();
           iter != seg_orphans.end(); ++iter) {
        matched += seg_stack->grab_max_overlap((*iter), gt_orphans, gt_rag, seg_matched, gt_matched);
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
     
    // ?? might give weird results when dilation is done on GT
    seg_stack->compute_groundtruth_assignment();
    vector<RagNode<Label>* > large_bodies;

    for (Rag<Label>::nodes_iterator iter = seg_rag->nodes_begin();
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
            int val = seg_stack->decide_edge_label(large_bodies[i], large_bodies[j]);

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

    // ?! get affinity pairs for missed node pairs
    

    std::tr1::unordered_map<Label, unsigned long long> body_changes;
    Rag<Label> opt_rag_copy(*opt_rag);

    bool change = true;
    while (change) {
        change = false;
        for (Rag<Label>::edges_iterator iter = opt_rag_copy.edges_begin();
                iter != opt_rag_copy.edges_end(); ++iter) {
            double weight = (*iter)->get_weight();
            if (weight < 0.0001) {
                RagNode<Label>* node1 = (*iter)->get_node1();
                RagNode<Label>* node2 = (*iter)->get_node2();

                unsigned long long large1 = node1->get_size();
                unsigned long long large2 = node2->get_size();
                if (body_changes.find(node1->get_node_id()) != body_changes.end()) {
                    large1 = body_changes[node1->get_node_id()];
                }
                if (body_changes.find(node2->get_node_id()) != body_changes.end()) {
                    large2 = body_changes[node2->get_node_id()];
                }
                RagNode<Label>* node_keep;
                if (large1 > large2) {
                    body_changes[node1->get_node_id()] = large1;
                    node_keep = node1;
                    body_changes.erase(node2->get_node_id());
                } else {
                    body_changes[node2->get_node_id()] = large2;
                    node_keep = node2;
                    body_changes.erase(node1->get_node_id());
                }

                vector<string> property_names;
                rag_merge_edge(opt_rag_copy, *iter,
                        node_keep, property_names); 

                change = true;
                break;
            }
        }
    }
    int num_undermerged_bodies = 0;
    for (std::tr1::unordered_map<Label, unsigned long long>::iterator iter = body_changes.begin();
            iter != body_changes.end(); ++iter) {
        unsigned long long largest_orig = iter->second;
        RagNode<Label>* node1 = opt_rag_copy.find_rag_node(iter->first);
        // TODO: should print size distribution since there is really nothing missed
        // higher above the threshold
        if ((node1->get_size() - largest_orig) >= options.body_error_size) {
            ++num_undermerged_bodies;
        } 
    }
    cout << "Number of under-merged bodies: " << num_undermerged_bodies << endl;
    
    // ?! count number of bodies where largest component body and size is greater than threshold
    

    // ?! get num of bodies that are >25 K off of GT, find connected regions over 25k from this??

   
    // ?! percentage of total size >25k, percentage misassigned and orphan (same with synapses) 
    // ?! print bodies (on both sides) that are off by threshold size
    // ?! print number of body/synapse errors above size threshold -- print path affinity/note exclusions
    // ?! print number of body/synapse errors that cumulatively cause problems
    // ?! print number of VI violators to threshold and root cause by affinity?
    cout << "************Finished Printing Statistics**************" << endl; 
}


void run_body(LocalEdgePriority<Label>& priority_scheduler, Json::Value json_vals,
        Stack* seg_stack, Stack* synapse_stack, Rag<Label>* opt_rag,
        int& num_modified, int& num_examined)
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

    get_num_edits(priority_scheduler, seg_stack, synapse_stack, opt_rag, num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Body mode finished ... "; 
}

void run_synapse(LocalEdgePriority<Label>& priority_scheduler, Json::Value json_vals,
        Stack* seg_stack, Stack* synapse_stack, Rag<Label>* opt_rag,
        int& num_modified, int& num_examined)
{
    double threshold = json_vals.get("threshold", 0.1).asDouble();
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;

    cout << endl << "Starting synapse mode with threshold " << threshold << endl;

    priority_scheduler.set_synapse_mode(threshold);

    get_num_edits(priority_scheduler, seg_stack, synapse_stack, opt_rag, num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Synapse mode finished ... "; 
}

void run_orphan(LocalEdgePriority<Label>& priority_scheduler, Json::Value json_vals,
        Stack* seg_stack, Stack* synapse_stack, Rag<Label>* opt_rag,
        int& num_modified, int& num_examined)
{
    unsigned int threshold = json_vals.get("threshold", 25000).asUInt();
    bool use_exclusions = json_vals.get("use-body-exclusions", false).asBool(); 
    num_modified = 0;
    num_examined = 0;

    ScopeTime timer;
    cout << endl << "Starting orphan mode with threshold " << threshold << endl;

    priority_scheduler.set_orphan_mode(threshold, 0, 0);

    get_num_edits(priority_scheduler, seg_stack, synapse_stack, opt_rag, num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Orphan mode finished ... "; 
}

void run_edge(LocalEdgePriority<Label>& priority_scheduler, Json::Value json_vals,
        Stack* seg_stack, Stack* synapse_stack, Rag<Label>* opt_rag,
        int& num_modified, int& num_examined)
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

    get_num_edits(priority_scheduler, seg_stack, synapse_stack, opt_rag, num_modified, num_examined, use_exclusions); 

    cout << "edges examined: " << num_examined << endl;
    cout << "edges modified: " << num_modified << endl;
    cout << "Edge mode finished ... "; 
}

void set_rag_probs(Stack* seg_stack, Rag<Label>* seg_rag) {
    for (Rag<Label>::edges_iterator iter = seg_rag->edges_begin();
            iter != seg_rag->edges_end(); ++iter) {
        // ?? mito mode a problem ??
        int val = seg_stack->decide_edge_label((*iter)->get_node1(), (*iter)->get_node2()); 
        if (val == -1) {
            (*iter)->set_weight(0.0);
        } else {
            (*iter)->set_weight(1.0);
        }
    }
}

void run_recipe(string recipe_filename, Stack* seg_stack, Stack* synapse_stack, Stack* gt_stack,
        Rag<Label>* opt_rag, AnalyzeGTOptions& options)
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

    Rag<Label>* rag = seg_stack->get_rag();

    Json::Value json_vals_priority;
    
    // load orphan information
    Json::Value orphan_bodies;
    unsigned int id = 0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if (!((*iter)->is_border())) {
            orphan_bodies[id++] = (*iter)->get_node_id(); 
        }
    }
    
    // load default information
    for (Rag<Label>::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        (*iter)->set_property("location", Location(0,0,0));
        (*iter)->set_property("edge_size", (*iter)->get_size());
    }
    
    json_vals_priority["orphan_bodies"] = orphan_bodies;

    // load synapse information
    std::tr1::unordered_map<Label, int> synapse_counts;
    seg_stack->load_synapse_counts(synapse_counts);
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

    LocalEdgePriority<Label> priority_scheduler(*rag, 0.1, 0.9, 0.1, json_vals_priority);
    
    int num_examined = 0;
    int num_modified = 0;

    for (int i = 0; i < json_vals["recipe"].size(); ++i) {
        Json::Value operation = json_vals["recipe"][i]; 
        string type = operation["type"].asString();

        // TODO: make a function map
        int num_examined2 = 0;
        int num_modified2 = 0;
        if (type == "body") {
            run_body(priority_scheduler, operation, seg_stack, synapse_stack,
                    opt_rag, num_modified2, num_examined2); 
        } else if (type == "synapse") {
            run_synapse(priority_scheduler, operation, seg_stack, synapse_stack,
                    opt_rag, num_modified2, num_examined2); 
        } else if (type == "edge") {
            run_edge(priority_scheduler, operation, seg_stack, synapse_stack,
                    opt_rag, num_modified2, num_examined2); 
        } else if (type == "orphan") {
            run_orphan(priority_scheduler, operation, seg_stack, synapse_stack,
                    opt_rag, num_modified2, num_examined2); 
        } else {
            throw ErrMsg("Unknow operation type: " + type);
        } 
        cout << endl;
         
        num_examined += num_examined2; 
        num_modified += num_modified2; 
        dump_differences(seg_stack, synapse_stack, gt_stack, opt_rag, options);
    } 

    cout << "Total edges examined: " << num_examined << endl;
    cout << "Total edges modified: " << num_modified << endl;

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

        //seg_filename, groundtruth_filename 
        int depth, height, width;
        Label* zp_labels = get_label_volume(options.seg_filename,
                options.enable_transforms, depth, height, width);
        cout << "Read seg stack" << endl;

        int depth2, height2, width2;
        Label* zp_gt_labels = get_label_volume(options.groundtruth_filename,
                options.enable_transforms, depth2, height2, width2);
        cout << "Read GT stack" << endl;

        if ((depth != depth2) || (height != height2) || (width != width2)) {
            throw ErrMsg("Mismatch in dimension sizes");
        }
      
        // create GT stack to get GT rag
        Stack* gt_stack = new Stack(zp_gt_labels, depth + 2*PADDING,
              height + 2*PADDING, width + 2*PADDING, PADDING);  
        gt_stack->build_rag();
        cout << "Built GT RAG" << endl;
        Rag<Label>* gt_rag = gt_stack->get_rag();  
        
        // create seg stack
        Stack* seg_stack = new Stack(zp_labels, depth + 2*PADDING,
              height + 2*PADDING, width + 2*PADDING, PADDING);  
        seg_stack->build_rag();
        cout << "Built seg RAG" << endl;
        Rag<Label>* seg_rag = seg_stack->get_rag();
       
        seg_stack->set_groundtruth(gt_stack->get_label_volume());
        seg_stack->compute_groundtruth_assignment();

        if (options.graph_filename != "") {
            // use previously generated probs
            Rag<Label>* seg_rag_probs = create_rag_from_jsonfile(options.graph_filename.c_str()); 
            if (!seg_rag_probs) {
                throw ErrMsg("Problem processing graph file");
            }

            for (Rag<Label>::edges_iterator iter = seg_rag_probs->edges_begin();
                    iter != seg_rag_probs->edges_end(); ++iter) {
                Label node1 = (*iter)->get_node1()->get_node_id();
                Label node2 = (*iter)->get_node2()->get_node_id();

                if (!(*iter)->is_false_edge()) {
                    double prob = (*iter)->get_weight();
                    RagEdge<Label>* edge = seg_rag->find_rag_edge(node1, node2);
                    edge->set_weight(prob);
                }
            }
            delete seg_rag_probs;
        } else {
            // use optimal probs
            set_rag_probs(seg_stack, seg_rag);
        }

        // create ground truth assignments to be maintained with rag
        Rag<Label>* rag_comp = new Rag<Label>(*seg_rag);
        set_rag_probs(seg_stack, rag_comp);

        // set synapse exclusions and create synapse stack for VI comparisons 
        Stack* synapse_stack = 0;
        if (options.synapse_filename != "") {
            vector<Label> seg_synapse_labels;
            vector<Label> gt_synapse_labels;

            seg_stack->set_exclusions(options.synapse_filename);
            gt_stack->set_exclusions(options.synapse_filename);
            seg_stack->compute_synapse_volume(seg_synapse_labels, gt_synapse_labels);

            unsigned int num_labels = seg_synapse_labels.size();
            Label * syn_labels2 = new Label[num_labels];
            Label * gt_labels2 = new Label[num_labels];

            for (int i = 0; i < seg_synapse_labels.size(); ++i) {
                syn_labels2[i] = seg_synapse_labels[i];
                gt_labels2[i] = gt_synapse_labels[i]; 
            }
            synapse_stack = new Stack(syn_labels2, 1, 1, seg_synapse_labels.size(), 0);
            synapse_stack->init_mappings();
            synapse_stack->set_groundtruth(gt_labels2);
        }

        // load body exclusions for comparative analysis (e.g., glia)
        if (options.exclusions_filename != "") {
            gt_stack->set_body_exclusions(options.exclusions_filename);
            seg_stack->set_groundtruth(gt_stack->get_label_volume());
            seg_stack->set_gt_exclusions();

        }

        // remove small bodies
        // TODO: handle synapse bodies
        if (options.min_filter_size > 0) {
            int num_removed = seg_stack->remove_small_regions(options.min_filter_size);
            cout << "Small seg bodies removed: " << num_removed << endl;
            num_removed = gt_stack->remove_small_regions(options.min_filter_size); 
            cout << "Small gt bodies removed: " << num_removed << endl;
        }

        // dilate edges for vi comparison
        if (options.gt_dilation > 0) {
            cout << "Create gt 0 boundaries" << endl;
            gt_stack->create_0bounds();
            for (int i = 1; i < options.gt_dilation; ++i) {
                cout << "Warning: dilation >1 not currently supported" << endl;
                // TODO: run dilation
            }
            seg_stack->set_groundtruth(gt_stack->get_label_volume());
        } 
        if (options.seg_dilation > 0) {
            seg_stack->create_0bounds();
            cout << "Created seg 0 boundaries" << endl;
            for (int i = 1; i < options.seg_dilation; ++i) {
                cout << "Warning: dilation >1 not currently supported" << endl;
                // TODO: run dilation
            }
        }        
        


        // print different statistics by default
        if (options.graph_filename != "") {
            // ?! print statistics on accuracy of initial predictions (histogram)
        }
        dump_differences(seg_stack, synapse_stack, gt_stack, rag_comp, options);

        // find gt body errors
        int body_errors = num_body_errors(gt_rag, options.body_error_size);
        cout << "Number of GT orphan bodies with more than " << options.body_error_size << 
            " voxels : " << body_errors << endl;

        // find gt synapse errors
        if (synapse_stack) { 
            int synapse_errors = num_synapse_errors(gt_stack, gt_rag, options.synapse_error_size); 
            cout << "Number of GT orphan bodies with more than " << options.synapse_error_size << 
                " synapse annotations : " << synapse_errors << endl;   
        }

        // try different strategies to refine the graph
        if (options.recipe_filename != "") {
            run_recipe(options.recipe_filename, seg_stack, synapse_stack,
                    gt_stack, rag_comp, options);
        }

        // dump final list of bad bodies if option is specified 
        if (options.dump_split_merge_bodies) {
            cout << "Showing body VI differences" << endl;
            seg_stack->dump_vi_differences(options.vi_threshold);
            if (synapse_stack) {
                cout << "Showing synapse body VI differences" << endl;
                synapse_stack->dump_vi_differences(options.vi_threshold);
            }
        }
        
        delete gt_stack;
    } catch (ErrMsg& err) {
        cerr << err.str << endl;
    }

    return 0;
}
