#include <Python.h>
#include <boost/python.hpp>

#include "init_numpy.h"
#include "converters.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <Utilities/ScopeTime.h>
#include <Classifier/vigraRFclassifier.h>
#include <Classifier/opencvRFclassifier.h>

#include <FeatureManager/FeatureMgr.h>
#include <BioPriors/BioStack.h>
#include <BioPriors/StackAgglomAlgs.h>

using namespace boost::python;
using namespace boost::algorithm;
using std::vector;
using std::cout; using std::endl;

namespace NeuroProof { namespace python {

// ?! build classifier in function for now -- use Classifier object in the future
// Note: always assumes mito is third channel
// TODO: allow segmentation constraints (e.g., synapses)
VolumeLabelPtr agglomerate(VolumeLabelPtr labels,
        vector<VolumeProbPtr> prob_array, std::string fn, double threshold)
{
    ScopeTime timer;
    // create classifier from file
    EdgeClassifier* eclfr;
    if (ends_with(fn, ".h5")) {
        eclfr = new VigraRFclassifier(fn.c_str());	
    } else if (ends_with(fn, ".xml")) {	
        eclfr = new OpencvRFclassifier(fn.c_str());	
    }
    
    // create feature manager and load classifier
    FeatureMgrPtr feature_manager(new FeatureMgr(prob_array.size()));
    feature_manager->set_basic_features(); 
    feature_manager->set_classifier(eclfr);   	 

    // create stack to hold segmentation state
    BioStack stack(labels); 
    stack.set_feature_manager(feature_manager);
    stack.set_prob_list(prob_array);

    // build graph
    stack.build_rag();
    cout<< "Initial number of regions: "<< stack.get_num_labels()<< endl;	

    // remove inclusions
    stack.remove_inclusions();
    cout<< "Regions after inclusion removal: "<< stack.get_num_labels()<< endl;	

    // perform agglomeration    
    agglomerate_stack(stack, threshold, true);
    cout<< "Regions after agglomeration: "<< stack.get_num_labels()<< endl;	
    stack.remove_inclusions();
    cout<< "Regions after inclusion removal: "<< stack.get_num_labels()<< endl;	
   
    agglomerate_stack_mito(stack);
    cout<< "Regions after mito agglomeration: "<< stack.get_num_labels()<< endl;	

    return stack.get_labelvol();
}

BOOST_PYTHON_MODULE(_agglomeration_python)
{
    init_numpy();
    ndarray_to_segmentation();
    ndarray_to_predictionarray(); 
 
    def("agglomerate" , agglomerate);
}


}}
