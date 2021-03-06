#ifndef ITERATIVE_LEARN_SEMI
#define ITERATIVE_LEARN_SEMI

#include "IterativeLearn.h"
#include "../SemiSupervised/weightmatrix1.h"
// #include "../SemiSupervised/weightmatrix2.h"
#include "../SemiSupervised/kmeans.h"
// #include "../Utilities/unique_row_matrix.h"
#include <Classifier/opencvRFclassifier.h>

// #define CHUNKSZ_SM 10 
// #define INITPCT_SM 0.035

#define INITIAL_METHOD_DEGREE 1
#define INITIAL_METHOD_KMEANS 2

namespace NeuroProof{

class IdxLbl{
public:
    unsigned int idx;
    int lbl;
    IdxLbl(unsigned int pidx, int plbl): idx(pidx), lbl(plbl){}
};
 

class IterativeLearn_semi:public IterativeLearn{

    WeightMatrix1* wt1;
//     WeightMatrix2* wt2;
    std::vector<unsigned int> trn_idx;
    std::vector<unsigned int> init_trn_idx;
    int initial_set_strategy;
    
    boost::thread* threadp;
    unsigned int nfeat_channels;
//     std::vector<unsigned int> nz_degree_idx;
//     std::vector<unsigned int> all_idx;
//     std::vector<unsigned int> tst_idx;
    const double INITPCT_SM ;
    const double CHUNKSZ_SM ;
    double w_dist_thd;
    int parallel_mode;
    
    std::map<unsigned int, double> m_prop_lbl;
    std::map<unsigned int, double> m_dis_pred;

public:
    IterativeLearn_semi(BioStack* pstack, string psession_name="", string pclfr_name="");
// {
// 	trn_idx.clear();
// 	edgelist.clear();
// 	initial_set_strategy = DEGREE;
// 	parallel_mode = 0;
// 	
// 	if (!(feature_mgr->get_classifier())){
// 	    EdgeClassifier* eclfr = new OpencvRFclassifier();
// 	  
// 	    feature_mgr->set_classifier(eclfr);
// 	}
// 	nfeat_channels = feature_mgr->get_num_channels();
// 	printf("num feat channels: %u\n", nfeat_channels);
// 	
// 	string param_filename= session_name + "/param_semi.txt";
// 	printf("%s\n",param_filename.c_str());
// 	FILE* fp = fopen(param_filename.c_str(),"rt");
// 	if(!fp)
// 	    printf("No param file for semi-supervised\n");
// 	else{
// 	    fscanf(fp, "%lf %lf %lf %d", &INITPCT_SM, &CHUNKSZ_SM, &w_dist_thd, &parallel_mode);
// 	    fclose(fp);
// 	    printf("initial # edge= %.5lf, chunksz=%.1lf, max weight dist=%.5lf \n", INITPCT_SM, CHUNKSZ_SM, w_dist_thd);
// 	}
	
//     }
    void get_initial_edges(std::vector<unsigned int>& new_idx);
    void read_saved_edges(string& psession_name, std::vector<unsigned int>& saved_idx, std::vector<int>& saved_lbl);   void learn_edge_classifier(double trnsz);
    void update_new_labels(std::vector<unsigned int>& new_idx, std::vector<int>& new_lbl);
    void get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx);

    
    std::vector<unsigned int>& get_trn_idx(){ return trn_idx;};
    void get_extra_edges(std::vector<unsigned int>& ret_idx, size_t nedges);
    void compute_new_risks(std::multimap<double, unsigned int>& risks);
    
    bool is_parallel_mode(){
	return (parallel_mode==1? true : false);
    }
    void save_classifier(string& psession_name);
    
};



}
#endif