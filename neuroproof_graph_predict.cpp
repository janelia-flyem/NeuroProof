#include "BioPriors/BioStackController.h"

#include "Utilities/ScopeTime.h"
#include "Utilities/OptionParser.h"
#include "Rag/RagIO.h"

#include "BioPriors/StackAgglom.h"


#include <boost/algorithm/string/predicate.hpp>
#include <cassert>
#include <iostream>


using namespace NeuroProof;

using std::cerr; using std::cout; using std::endl;
using std::string;
using std::vector;
using namespace boost::algorithm;
using std::tr1::unordered_set;

static const char * SEG_DATASET_NAME = "stack";
static const char * PRED_DATASET_NAME = "volume/predictions";

void remove_inclusions(BioStackController& stack_controller)
{
    cout<<"Inclusion removal ..."; 
    stack_controller.remove_inclusions();
    cout<<"done with "<< stack_controller.get_num_labels()<< " nodes\n";	
}

struct PredictOptions
{
    PredictOptions(int argc, char** argv) : synapse_filename(""), output_filename("segmentation.h5"),
        graph_filename("graph.json"), threshold(0.2), watershed_threshold(0), post_synapse_threshold(0.0),
        merge_mito(true), agglo_type(1), enable_transforms(true), postseg_classifier_filename(""),
        location_prob(true)
    {
        OptionParser parser("Program that predicts edge confidence for a graph and merges confident edges");

        // positional arguments
        parser.add_positional(watershed_filename, "watershed-file",
                "gala h5 file with label volume (z,y,x) and body mappings"); 
        parser.add_positional(prediction_filename, "prediction-file",
                "ilastik h5 file (x,y,z,ch) that has pixel predictions"); 
        parser.add_positional(classifier_filename, "classifier-file",
                "opencv or vigra agglomeration classifier"); 

        // optional arguments
        parser.add_option(synapse_filename, "synapse-file",
                "json file that contains synapse annotations that are used as constraints in merging"); 
        parser.add_option(output_filename, "output-file",
                "h5 file that will contain the output segmentation (z,y,x) and body mappings"); 
        parser.add_option(graph_filename, "graph-file",
                "json file that will contain the output graph"); 
        parser.add_option(threshold, "threshold",
                "segmentation threshold"); 
        parser.add_option(watershed_threshold, "watershed-threshold",
                "threshold used for removing small bodies as a post-process step"); 
        parser.add_option(postseg_classifier_filename, "postseg-classifier-file",
                "opencv or vigra agglomeration classifier to be used after agglomeration to assign confidence to the graph edges -- classifier-file used if not specified"); 
        parser.add_option(post_synapse_threshold, "post-synapse-threshold",
                "Merge synapses indepedent of constraints"); 

        // invisible arguments
        parser.add_option(merge_mito, "merge-mito",
                "perform separate mitochondrion merge phase", true, false, true); 
        parser.add_option(agglo_type, "agglo-type",
                "merge mode used", true, false, true); 
        parser.add_option(enable_transforms, "transforms",
                "enables using the transforms table when reading the segmentation", true, false, true); 
        parser.add_option(location_prob, "location_prob",
                "enables pixel prediction when choosing optimal edge location", true, false, true); 

        parser.parse_options(argc, argv);
    }

    // manadatory positionals
    string watershed_filename;
    string prediction_filename;
    string classifier_filename;
   
    // optional (with default values) 
    string synapse_filename;
    string output_filename;
    string graph_filename;

    double threshold;
    int watershed_threshold; // might be able to increase default to 500
    string postseg_classifier_filename;
    double post_synapse_threshold;

    // hidden options (with default values)
    bool merge_mito;
    int agglo_type;
    bool enable_transforms;
    bool location_prob;
};


void run_prediction(PredictOptions& options)
{
    // create prediction array
    vector<VolumeProbPtr> prob_list = VolumeProb::create_volume_array(
        options.prediction_filename.c_str(), PRED_DATASET_NAME);
    VolumeProbPtr boundary_channel = prob_list[0];
    cout << "Read prediction array" << endl;

    // create watershed volume
    VolumeLabelPtr initial_labels = VolumeLabelData::create_volume(
            options.watershed_filename.c_str(), SEG_DATASET_NAME);
    cout << "Read watershed" << endl;

    
    // TODO: move feature handling to controller (load classifier if file provided)
    // create feature manager and load classifier
    FeatureMgrPtr feature_manager(new FeatureMgr(prob_list.size()));
    feature_manager->set_basic_features(); 

    EdgeClassifier* eclfr;
    if (ends_with(options.classifier_filename, ".h5"))
    	eclfr = new VigraRFclassifier(options.classifier_filename.c_str());	
    else if (ends_with(options.classifier_filename, ".xml")) 	
	eclfr = new OpencvRFclassifier(options.classifier_filename.c_str());	

    feature_manager->set_classifier(eclfr);   	 

    // create stack to hold segmentation state
    BioStack stack(initial_labels); 
    stack.set_feature_manager(feature_manager);
    stack.set_prob_list(prob_list);

    // Controller with mito and synapse capability
    BioStackController stack_controller(&stack);

    cout<<"Building RAG ..."; 	
    stack_controller.build_rag_mito();
    cout<<"done with "<< stack_controller.get_num_labels()<< " nodes\n";	
   
    // add synapse constraints (send json to stack function)
    if (options.synapse_filename != "") {   
        stack_controller.set_synapse_exclusions(options.synapse_filename.c_str());
    }
    
    remove_inclusions(stack_controller);
    
    switch (options.agglo_type) {
        case 0: 
            cout<<"Agglomerating (flat) upto threshold "<< options.threshold<< " ..."; 
            agglomerate_stack_flat(stack_controller, options.threshold, options.merge_mito);
            break;
        case 1:
            cout<<"Agglomerating (agglo) upto threshold "<< options.threshold<< " ..."; 
            agglomerate_stack(stack_controller, options.threshold, options.merge_mito);
            break;        
        case 2:
            cout<<"Agglomerating (mrf) upto threshold "<< options.threshold<< " ..."; 
            agglomerate_stack_mrf(stack_controller, options.threshold, options.merge_mito);
            break;
        case 3:
            cout<<"Agglomerating (queue) upto threshold "<< options.threshold<< " ..."; 
            agglomerate_stack_queue(stack_controller, options.threshold, options.merge_mito);
            break;
        case 4:
            cout<<"Agglomerating (flat) upto threshold "<< options.threshold<< " ..."; 
            agglomerate_stack_flat(stack_controller, options.threshold, options.merge_mito);
            break;
        default: throw ErrMsg("Illegal agglomeration type specified");
    }
    cout << "Done with "<< stack_controller.get_num_labels()<< " regions\n";
   
    if (options.post_synapse_threshold > 0.00001) {
        cout << "Agglomerating (agglo) ignoring synapse constraints upto threshold "
            << options.post_synapse_threshold << endl;
        string dummy1, dummy2;
        agglomerate_stack(stack_controller, options.post_synapse_threshold,
                    options.merge_mito, false, true);
        cout << "Done with "<< stack_controller.get_num_labels() << " regions\n";
    }
    
    remove_inclusions(stack_controller);

    if (options.merge_mito){
	cout<<"Merge Mitochondria (border-len) ..."; 
        agglomerate_stack_mito(stack_controller);
    	cout<<"done with "<< stack_controller.get_num_labels() << " regions\n";	

        remove_inclusions(stack_controller);        
    } 	

    if (options.watershed_threshold > 0) {
        cout << "Removing small bodies ... ";

        unordered_set<Label_t> synapse_labels;
        stack_controller.load_synapse_labels(synapse_labels);
        int num_removed = stack_controller.absorb_small_regions(boundary_channel,
                        options.watershed_threshold, synapse_labels);
        cout << num_removed << " removed" << endl;	
    }

    if (options.postseg_classifier_filename == "") {
        options.postseg_classifier_filename = options.classifier_filename;
    }

    delete eclfr;
    if (ends_with(options.postseg_classifier_filename, ".h5"))
    	eclfr = new VigraRFclassifier(options.postseg_classifier_filename.c_str());	
    else if (ends_with(options.postseg_classifier_filename, ".xml")) 	
	eclfr = new OpencvRFclassifier(options.postseg_classifier_filename.c_str());	
    
    feature_manager->clear_features();
    feature_manager->set_classifier(eclfr);   	 
    stack_controller.build_rag();
    

    // add synapse constraints (send json to stack function)
    if (options.synapse_filename != "") {   
        stack_controller.set_synapse_exclusions(options.synapse_filename.c_str());
    }

    stack_controller.serialize_stack(options.output_filename.c_str(),
                options.graph_filename.c_str(), options.location_prob);

    delete eclfr;
}

int main(int argc, char** argv) 
{
    PredictOptions options(argc, argv);
    ScopeTime timer;

    run_prediction(options);

    return 0;
}


