/*!
 * \file
 * Deprecated:  This file contains functionality for creating and exporting
 * a graph file to be use in a particular, non-generic workflow.  This
 * code will not be maintained further but is kept in for legacy applications
 * requiring it.
 * 
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#include "../FeatureManager/FeatureMgr.h"

#include "../Stack/Stack.h"

#include "../Utilities/ScopeTime.h"
#include "../Utilities/OptionParser.h"
#include "../Rag/RagIO.h"

#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

using namespace NeuroProof;

using std::cerr; using std::cout; using std::endl;
using std::string;
using std::vector;
using namespace boost::algorithm;
using std::tr1::unordered_set;

static const char * SEG_DATASET_NAME = "stack";
static const char * PRED_DATASET_NAME = "volume/predictions";

/*!
 * Options for creating a graph for use in the Raveler tool
*/
struct SpOptions
{
    SpOptions(int argc, char** argv) : start_plane(0)
    {
        OptionParser parser("Program that creates a graph from Raveler superpixels");

        // positional arguments
        parser.add_positional(segmentation_filename, "segmentation-h5",
                "initial segmentation labels"); 
        parser.add_positional(prediction_filename, "prediction-file",
                "ilastik h5 file (x,y,z,ch) that has pixel predictions"); 
        parser.add_option(start_plane, "starting-zplane",
                "First z plane in Raveler space");
        
        parser.parse_options(argc, argv);
    }

    // manadatory positionals
    
    //! h5 file for segmentation (z,y,x)
    string segmentation_filename;

    //! h5 file with prediction (x,y,z,ch)
    string prediction_filename;

    //! starting z image plane 
    int start_plane;
};

/*!
 * Using the program options, create a graph and export in
 * json format that computes an edge weight between nodes based
 * on the median feature of the first prediction channel
 * and splits supervoxel into sets of superpixel planes.
*/
void generate_sp_graph(SpOptions& options)
{
    // create prediction array
    vector<VolumeProbPtr> prob_list = VolumeProb::create_volume_array(
        options.prediction_filename.c_str(), PRED_DATASET_NAME);
    VolumeProbPtr boundary_channel = prob_list[0];
    //(*boundary_channel) += (*(prob_list[2]));
    cout << "Read prediction array" << endl;

    VolumeLabelPtr raveler_labels = VolumeLabelData::create_volume(
            options.segmentation_filename.c_str(), SEG_DATASET_NAME);
   
    cout << "Read watershed" << endl;

 
    prob_list.clear();
    // only use boundary channel
    prob_list.push_back(boundary_channel);
    FeatureMgrPtr feature_manager2(new FeatureMgr(prob_list.size()));
    feature_manager2->add_median_feature(); 

    // map supervoxel ids to superpixel ids
    // maximum allowable plane ID 200,000; maxium number of planes, 10,000!!
    volume_forXYZ((*raveler_labels), x, y, z) {
        if ((*raveler_labels)(x,y,z) != 0) {
            unsigned int zpos = z + options.start_plane;
            raveler_labels->set(x, y, z, (zpos*200000) + (*raveler_labels)(x,y,z));
        } 
    }

    // create stack to hold segmentation state
    Stack stack2(raveler_labels); 
    stack2.set_feature_manager(feature_manager2);
    stack2.set_prob_list(prob_list);

    cout<<"Building RAG ..."; 	
    stack2.Stack::build_rag();
    cout<<"done with "<< stack2.get_num_labels()<< " nodes\n";	
  
    RagPtr rag = stack2.get_rag();
    
    // set the weight to edges within the superpixel set to -1 
    for (Rag_t::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        double val = feature_manager2->get_prob((*iter));
        (*iter)->set_weight(val);

        Node_t node1 = (*iter)->get_node1()->get_node_id();
        Node_t node2 = (*iter)->get_node2()->get_node_id();

        node1 = node1 % 200000; 
        node2 = node2 % 200000;
        if (node1 == node2) {
            (*iter)->set_weight(-1.0);
        } 
    }
 
    // write out graph json
    ofstream fout("graph.txt");
    
    for (Rag_t::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        fout << (*iter)->get_node1()->get_node_id() << " "; 
        fout << (*iter)->get_node2()->get_node_id() << " ";
        fout << (*iter)->get_node1()->get_size() << " "; 
        fout << (*iter)->get_node2()->get_size() << " ";
        fout << (*iter)->get_weight() << " ";
        fout << (*iter)->get_size() << endl;
    }

    fout.close();
}

/*!
 * Entry point for program, determines the program options
 * and calls the graph generation algorithm.
*/
int main(int argc, char** argv) 
{
    SpOptions options(argc, argv);
    ScopeTime timer;

    generate_sp_graph(options);

    return 0;
}


