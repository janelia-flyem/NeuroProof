#ifndef ITERATIVE_LEARN_UNC
#define ITERATIVE_LEARN_UNC

#include "IterativeLearn.h"
// #include "../Utilities/unique_row_matrix.h"




// #define CHUNKSZ 10 
//#define INITPCT 0.025


namespace NeuroProof{

 

class IterativeLearn_uncertain:public IterativeLearn{

    std::vector<unsigned int> trn_idx;
    std::vector<int> trn_lbl;
    std::vector<unsigned int> init_trn_idx;
    
//     boost::thread* threadp;
    const double INITPCT;
    const double CHUNKSZ;
public:
    IterativeLearn_uncertain(BioStack* pstack, string pclfr_name=""): IterativeLearn(pstack, pclfr_name), 
				      INITPCT(0.025), CHUNKSZ(10){
	trn_idx.clear();
	init_trn_idx.clear();
// 	INITPCT = 0.025;
    }
    void get_initial_edges(std::vector<unsigned int>& new_idx);
    void update_new_labels(std::vector<unsigned int>& new_idx, std::vector<int>& new_lbl);
    void get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx);

    void learn_edge_classifier(double trnsz);
    
};



}
#endif