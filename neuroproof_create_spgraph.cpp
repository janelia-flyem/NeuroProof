#include "Stack/StackController.h"
#include "Stack/Stack.h"

#include "Utilities/ScopeTime.h"
#include "Utilities/OptionParser.h"
#include "Rag/RagIO.h"

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
    string segmentation_filename;
    string prediction_filename;
    int start_plane;
};

void generate_sp_graph(SpOptions& options)
{
    // create prediction array
    vector<VolumeProbPtr> prob_list = VolumeProb::create_volume_array(
        options.prediction_filename.c_str(), PRED_DATASET_NAME);
    VolumeProbPtr boundary_channel = prob_list[0];
    prob_list.clear();
    prob_list.push_back(boundary_channel);
    cout << "Read prediction array" << endl;

    VolumeLabelPtr raveler_labels = VolumeLabelData::create_volume(
            options.segmentation_filename.c_str(), SEG_DATASET_NAME, false);
    
    volume_forXYZ((*raveler_labels), x, y, z) {
        if ((*raveler_labels)(x,y,z) != 0) {
            unsigned int zpos = z + options.start_plane;
            raveler_labels->set(x, y, z, (zpos << 16) + (*raveler_labels)(x,y,z));
        } 
    }   
    cout << "Read watershed" << endl;

   
    // TODO: move feature handling to controller (load classifier if file provided)
    // create feature manager and load classifier
    FeatureMgrPtr feature_manager(new FeatureMgr(prob_list.size()));
    feature_manager->add_median_feature(); 

    
    // create stack to hold segmentation state
    Stack2 stack(raveler_labels); 
    stack.set_feature_manager(feature_manager);
    stack.set_prob_list(prob_list);

    // Controller with mito and synapse capability
    StackController stack_controller(&stack);

    cout<<"Building RAG ..."; 	
    stack_controller.build_rag();
    cout<<"done with "<< stack_controller.get_num_labels()<< " nodes\n";	
  
    RagPtr rag = stack.get_rag();
 
    for (Rag_uit::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        double val = feature_manager->get_prob((*iter));
        (*iter)->set_weight(val);

        Node_uit node1 = (*iter)->get_node1()->get_node_id();
        Node_uit node2 = (*iter)->get_node2()->get_node_id();

        node1 = 0xffff & node1;
        node2 = 0xffff & node2;

        if (node1 == node2) {
            (*iter)->set_weight(-1.0);
        }
    }
 
    Json::Value json_writer;
    create_json_from_rag(rag.get(), json_writer, false);
    // write out graph json
    ofstream fout("graph.json");
    fout << json_writer;
    fout.close();
}

int main(int argc, char** argv) 
{
    SpOptions options(argc, argv);
    ScopeTime timer;

    generate_sp_graph(options);

    return 0;
}


