#ifndef ITERATIVE_LEARN_
#define ITERATIVE_LEARN_

#include "BioStack.h"
#include "Dataset.h"
#include <boost/thread.hpp>
#include <boost/ref.hpp>


#include <float.h>
#include <cstdlib>

#include "../FeatureManager/FeatureMgr.h"

#define IVAL 500


namespace NeuroProof{


class IterativeLearn
{

protected:
//     UniqueRowFeature_Label all_featuresu;
//     std::vector<int> all_labels;  
//     std::vector< std::vector<double> > all_features;
    Dataset dtst;
    
    BioStack* stack;
    RagPtr rag;
    FeatureMgrPtr feature_mgr;

    std::vector< std::vector<double> > cum_train_features;
    std::vector< int > cum_train_labels;
    std::vector< std::vector<double> > rest_features;
    std::vector< int > rest_labels;

    
    string clfr_name; 
    
    bool use_mito;
    
    std::vector< std::pair<Node_t, Node_t> > edgelist;
public:
  
    IterativeLearn(BioStack* pstack, string pclfr_name=""){
	stack = pstack;
	rag = stack->get_rag();
	feature_mgr = stack->get_feature_manager();
	clfr_name = pclfr_name;
	
	use_mito = true;
    };

    
    
    
    
    virtual void get_initial_edges(std::vector<unsigned int>& new_idx) = 0;
    virtual void update_new_labels(std::vector<unsigned int>& new_idx, 
				   std::vector<int>& new_lbl) = 0;
    virtual void get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx) = 0;
    
    
    virtual void learn_edge_classifier(double trnsz){};
    
    
    //void learn_edge_classifier_flat_random(double trnsz);

    void edgelist_from_index(std::vector<unsigned int>& new_idx,
			      std::vector< std::pair<Node_t, Node_t> >& elist);

    void compute_all_edge_features(std::vector< std::vector<double> >& all_features,
					      std::vector<int>& all_labels);

    void evaluate_accuracy(std::vector< std::vector<double> >& test_features,
			       std::vector<int>& test_labels, double thd );
    void evaluate_accuracy(std::vector<int>& labels, std::vector<double>& predicted_vals, double thd);

    void update_clfr_name(string &clfr_name, size_t trnsz);

  
};





}
#endif
