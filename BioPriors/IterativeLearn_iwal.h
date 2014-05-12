#ifndef ITERATIVE_LEARN_IWAL
#define ITERATIVE_LEARN_IWAL

#include "IterativeLearn.h"
// #include "../Utilities/unique_row_matrix.h"
#include "../Classifier/opencvRFclassifier.h"
#include <map>



// #define CHUNKSZ 10 
// #define INITPCT 0.1


namespace NeuroProof{

 

class IterativeLearn_iwal:public IterativeLearn{

    std::vector<unsigned int> trn_idx;
    std::vector<int> trn_lbl;
    std::vector<unsigned int> init_trn_idx;
    
    size_t nclassifiers;
    std::vector<OpencvRFclassifier*> clfr_pool;
//     boost::thread* threadp;
    std::vector<double> prob;
    
    std::map<unsigned int, double> wt_trn_idx;
    double pmin;
    
    const double INITPCT;
    const double CHUNKSZ;
public:
    IterativeLearn_iwal(BioStack* pstack, string pclfr_name=""): IterativeLearn(pstack, pclfr_name), 
					    INITPCT(0.1), CHUNKSZ(10){
	trn_idx.clear();
	init_trn_idx.clear();
	nclassifiers = 10 ;
	pmin = 0.1;
    }
    void get_initial_edges(std::vector<unsigned int>& new_idx);
    void update_new_labels(std::vector<unsigned int>& new_idx, std::vector<int>& new_lbl);
    void get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx);
    void learn_edge_classifier(double trnsz);

    
    void get_bootstrap_idx(size_t len, std::vector<unsigned int>& retvec);
    void get_bootstrap_trset(std::vector<unsigned int >& bootstrap_idx,
				      std::vector< std::vector<double> >& bootstrap_features,
				      std::vector< int >& bootstrap_labels);
    double compute_max_loss_deviance(std::vector<double>& sample);
    double compute_max_pred_deviance(std::vector<double>& sample);
    void rejection_sample(size_t len, std::vector<unsigned int>& add_idx );
    
};



}
#endif