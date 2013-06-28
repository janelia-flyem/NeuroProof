#include "BioPriors/BioStackController.h"
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
        parser.add_positional(classifier_filename, "classifier-file",
                "opencv or vigra agglomeration classifier"); 
        
        parser.parse_options(argc, argv);
    }

    // manadatory positionals
    string segmentation_filename;
    string prediction_filename;
    string classifier_filename;
    int start_plane;
};

void generate_sp_graph(SpOptions& options)
{
    // create prediction array
    vector<VolumeProbPtr> prob_list = VolumeProb::create_volume_array(
        options.prediction_filename.c_str(), PRED_DATASET_NAME);
    VolumeProbPtr boundary_channel = prob_list[0];
    //(*boundary_channel) += (*(prob_list[2]));
    cout << "Read prediction array" << endl;

    VolumeLabelPtr raveler_labels = VolumeLabelData::create_volume(
            options.segmentation_filename.c_str(), SEG_DATASET_NAME, false);
   
    cout << "Read watershed" << endl;

  
    FeatureMgrPtr feature_manager(new FeatureMgr(prob_list.size()));
    feature_manager->set_basic_features(); 

    EdgeClassifier* eclfr;
    if (ends_with(options.classifier_filename, ".h5"))
    	eclfr = new VigraRFclassifier(options.classifier_filename.c_str());	
    else if (ends_with(options.classifier_filename, ".xml")) 	
	eclfr = new OpencvRFclassifier(options.classifier_filename.c_str());	
    feature_manager->set_classifier(eclfr);   	 

    BioStack stack(raveler_labels); 
    stack.set_feature_manager(feature_manager);
    stack.set_prob_list(prob_list);
    BioStackController stack_controller(&stack);
    cout<<"Building RAG ..."; 	
    stack_controller.build_rag_mito();
    cout<<"done with "<< stack_controller.get_num_labels()<< " nodes\n";	




    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    prob_list.clear();
    prob_list.push_back(boundary_channel);
    FeatureMgrPtr feature_manager2(new FeatureMgr(prob_list.size()));
    feature_manager2->add_median_feature(); 

    // ?! do a max z and a max label check for encoding 
    volume_forXYZ((*raveler_labels), x, y, z) {
        if ((*raveler_labels)(x,y,z) != 0) {
            unsigned int zpos = z + options.start_plane;
            raveler_labels->set(x, y, z, (zpos*200000) + (*raveler_labels)(x,y,z));
        } 
    }

    // create stack to hold segmentation state
    Stack2 stack2(raveler_labels); 
    stack2.set_feature_manager(feature_manager2);
    stack2.set_prob_list(prob_list);

    // Controller with mito and synapse capability
    StackController stack_controller2(&stack2);

    cout<<"Building RAG ..."; 	
    stack_controller2.build_rag();
    cout<<"done with "<< stack_controller2.get_num_labels()<< " nodes\n";	
  
    RagPtr rag = stack2.get_rag();
    RagPtr rag_base = stack.get_rag();
           
    for (Rag_uit::edges_iterator iter = rag_base->edges_begin();
            iter != rag_base->edges_end(); ++iter) {
        //double weight = feature_manager->get_prob(*iter);
        double weight = 1.0;
        if ((stack_controller.is_mito((*iter)->get_node1()->get_node_id())) !=
                (stack_controller.is_mito((*iter)->get_node2()->get_node_id()))) {
            weight = -1.0;
        }
        (*iter)->set_weight(weight);
    }
    cout << "Examined mitos" << endl;

    for (Rag_uit::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        double val = feature_manager2->get_prob((*iter));
        //(*iter)->set_weight(val);

        Node_uit node1 = (*iter)->get_node1()->get_node_id();
        Node_uit node2 = (*iter)->get_node2()->get_node_id();

        node1 = node1 % 200000; 
        node2 = node2 % 200000;
        if (node1 == node2) {
            (*iter)->set_weight(-1.0);
        } else { 
            RagNode_uit* rn1 = rag_base->find_rag_node(node1);
            RagNode_uit* rn2 = rag_base->find_rag_node(node2);

            assert(rn1 && rn2);
            RagEdge_uit* rag_edge = rag_base->find_rag_edge(rn1, rn2);
            assert(rag_edge);
            if (rag_edge->get_weight() < 0) {
                //val += (1 - val) / 2;
            }
            (*iter)->set_weight(val);
        } 
    }
 
    //Json::Value json_writer;
    //create_json_from_rag(rag.get(), json_writer, false);
    // write out graph json
    ofstream fout("graph.txt");
    
    for (Rag_uit::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        fout << (*iter)->get_node1()->get_node_id() << " "; 
        fout << (*iter)->get_node2()->get_node_id() << " ";
        fout << (*iter)->get_node1()->get_size() << " "; 
        fout << (*iter)->get_node2()->get_size() << " ";
        fout << (*iter)->get_weight() << " ";
        fout << (*iter)->get_size() << endl;
    }

    //fout << json_writer;
    fout.close();
}

int main(int argc, char** argv) 
{
    SpOptions options(argc, argv);
    ScopeTime timer;

    generate_sp_graph(options);

    return 0;
}


