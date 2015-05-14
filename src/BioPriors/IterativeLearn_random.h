
#ifndef ITERATIVE_LEARN_RND
#define ITERATIVE_LEARN_RND


#include "IterativeLearn.h"

namespace NeuroProof{

class IterativeLearn_rnd: public IterativeLearn {
  
public:
    IterativeLearn_rnd(BioStack* pstack, string pclfr_name=""): IterativeLearn(pstack, pclfr_name){};
      
    void get_initial_edges(std::vector<unsigned int>& new_idx);
    void learn_edge_classifier(double trnsz);
  
    void update_new_labels(std::vector<unsigned int>& new_idx, std::vector<int>& new_lbl){}
    void get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx){}
};

void IterativeLearn_rnd::get_initial_edges(std::vector<unsigned int>& new_idx){
  
    std::vector< std::vector<double> >& all_features = dtst.get_features();
    std::vector< int >& all_labels = dtst.get_labels();
    compute_all_edge_features(all_features, all_labels);
    dtst.initialize();
    
//     std::srand ( unsigned ( std::time(0) ) );
//     size_t chunksz = CHUNKSZ;
//     size_t nsamples = all_features.size();
//     
//     size_t start_with = (size_t) (nsamples);

    std::srand ( unsigned ( std::time(0) ) );
    new_idx.clear();
    for(size_t ii=0; ii < all_features.size(); ii++)
	new_idx.push_back(ii);
    
    std::random_shuffle(new_idx.begin(), new_idx.end());	
    
//     dtst.get_train_data(new_idx, cum_train_features, cum_train_labels);
    
    
}



void IterativeLearn_rnd::learn_edge_classifier(double trnsz){
    
    std::vector<unsigned int> new_idx;

    get_initial_edges(new_idx);
    
    if (trnsz > 0)
	new_idx.erase(new_idx.begin()+trnsz, new_idx.end());
    
    std::vector<int> new_lbl(new_idx.size());
    std::vector< int >& all_labels = dtst.get_labels();
    for(size_t ii=0; ii < new_idx.size() ; ii++){
	unsigned int idx = new_idx[ii];
	new_lbl[ii] = all_labels[idx];
    }
    
    dtst.set_trn_labels(new_lbl);
    dtst.get_train_data(new_idx, cum_train_features, cum_train_labels);
    feature_mgr->get_classifier()->learn(cum_train_features, cum_train_labels); // number of trees
    dtst.get_test_data(new_idx, rest_features, rest_labels);  
    
    
//     std::vector< std::vector<double> >& all_features = dtst.get_features();
//     std::vector< int >& all_labels = dtst.get_labels();
//     compute_all_edge_features(all_features, all_labels);
//     dtst.initialize();
//     
//     size_t nexamples2learn = trnsz;
//     dtst.get_random_train_test_data(nexamples2learn,cum_train_features, cum_train_labels,
// 		    rest_features, rest_labels);
//     
//     printf("Features generated\n");
//     feature_mgr->get_classifier()->learn(cum_train_features, cum_train_labels); // number of trees
//     printf("Classifier learned \n");

    double thd_r= 0.3;
    if (rest_features.size()>0)
	evaluate_accuracy(rest_features,rest_labels, thd_r);

}

}
#endif