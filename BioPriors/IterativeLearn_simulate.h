#ifndef ITERATIVE_LEARN_SIMULATE
#define ITERATIVE_LEARN_SIMULATE

#include "IterativeLearn.h"
#include <cstdio>
#include "../SemiSupervised/weightmatrix1.h"
#include "../SemiSupervised/kmeans.h"
// #include "../Utilities/unique_row_matrix.h"




// #define CHUNKSZ_SM 10 
// #define INITPCT_SM 0.035

#define DEGREE 1
#define KMEANS 2

namespace NeuroProof{

 

class IterativeLearn_simulate:public IterativeLearn{

    WeightMatrix1* wt1;
    std::vector<unsigned int> trn_idx;
    std::vector<unsigned int> init_trn_idx;
    int initial_set_strategy;
    
    boost::thread* threadp;
//     std::vector<unsigned int> nz_degree_idx;
//     std::vector<unsigned int> all_idx;
//     std::vector<unsigned int> tst_idx;
    const double INITPCT_SM ;
    const double CHUNKSZ_SM ;
public:
    IterativeLearn_simulate(BioStack* pstack, string pclfr_name=""): IterativeLearn(pstack, pclfr_name),
				  INITPCT_SM(0.035), CHUNKSZ_SM(10){
	trn_idx.clear();
	edgelist.clear();
	initial_set_strategy = DEGREE;
	
    }
    void get_initial_edges(std::vector<unsigned int>& new_idx);
    void learn_edge_classifier(double trnsz);
    void update_new_labels(std::vector<unsigned int>& new_idx, std::vector<int>& new_lbl);
    void get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx);

    
    std::vector<unsigned int>& get_trn_idx(){ return trn_idx;};
    void get_extra_edges(std::vector<unsigned int>& ret_idx, size_t nedges);
    void compute_new_risks(std::multimap<double, unsigned int>& risks, 
			   std::map<unsigned int, double>& prop_lbl);
    
};



}
#endif