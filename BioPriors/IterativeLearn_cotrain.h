#ifndef ITERATIVE_LEARN_COTRAIN
#define ITERATIVE_LEARN_COTRAIN

#include "IterativeLearn.h"
#include "../Classifier/opencvRFclassifier.h"
#include <map>



// #define CHUNKSZ 10 
// #define INITPCT 0.1


namespace NeuroProof{

 

class IterativeLearn_co:public IterativeLearn{

    std::vector<unsigned int> trn_idx;
    std::vector<int> trn_lbl;
    std::vector<unsigned int> init_trn_idx;
    
    size_t nclassifiers;
    std::vector<OpencvRFclassifier*> clfr_pool;
    std::vector< std::vector<unsigned int> > indiv_trn_idx;
    std::vector< std::vector<int> > indiv_trn_lbl;
//     boost::thread* threadp;
    std::vector<double> prob;
    
    std::map<double, unsigned int> risks;
    double pmin;
    
    const double INITPCT;
    const double CHUNKSZ;
public:
    IterativeLearn_co(BioStack* pstack, string pclfr_name=""): IterativeLearn(pstack, pclfr_name), 
					    INITPCT(0.01), CHUNKSZ(10){
	trn_idx.clear();
	init_trn_idx.clear();
	nclassifiers = 2 ;
	pmin = 0.1;
	indiv_trn_idx.resize(nclassifiers);
	indiv_trn_lbl.resize(nclassifiers);
	for(size_t ii=0; ii< nclassifiers; ii++)
	    clfr_pool.push_back(new OpencvRFclassifier);
	
    }
    void get_initial_edges(std::vector<unsigned int>& new_idx);
    void update_new_labels(std::vector<unsigned int>& new_idx, std::vector<int>& new_lbl);
    void get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx);
    void learn_edge_classifier(double trnsz);

    
    double compute_max_loss_deviance(std::vector<double>& sample);
    double compute_max_pred_deviance(std::vector<double>& sample);
    void compute_new_risks();
    void update_indiv_trnset(std::vector<unsigned int>& new_idx,
					    std::vector<int>& new_lbl);
    
};



}
#endif