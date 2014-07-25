#ifndef ITERATIVE_LEARN_SEMI
#define ITERATIVE_LEARN_SEMI

#include "IterativeLearn.h"
#include "../SemiSupervised/weightmatrix1.h"
#include "../SemiSupervised/kmeans.h"
// #include "../Utilities/unique_row_matrix.h"




// #define CHUNKSZ_SM 10 
// #define INITPCT_SM 0.035

#define DEGREE 1
#define KMEANS 2

namespace NeuroProof{

 

class IterativeLearn_semi:public IterativeLearn{

    WeightMatrix1* wt1;
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

public:
    IterativeLearn_semi(BioStack* pstack, string session_name="", string pclfr_name=""): IterativeLearn(pstack, pclfr_name),
				  INITPCT_SM(0.035), CHUNKSZ_SM(10), w_dist_thd(5){
	trn_idx.clear();
	edgelist.clear();
	initial_set_strategy = DEGREE;
	
	if (!(feature_mgr->get_classifier())){
	    EdgeClassifier* eclfr = new OpencvRFclassifier();
	  
	    feature_mgr->set_classifier(eclfr);
	}
	nfeat_channels = feature_mgr->get_num_channels();
	printf("num feat channels: %u\n", nfeat_channels);
	
	string param_filename= session_name + "/param_semi.txt";
	printf("%s\n",param_filename.c_str());
	FILE* fp = fopen(param_filename.c_str(),"rt");
	if(!fp)
	    printf("No param file for semi-supervised\n");
	else{
	    fscanf(fp, "%lf %lf %lf %d", &INITPCT_SM, &CHUNKSZ_SM, &w_dist_thd, &parallel_mode);
	    fclose(fp);
	    printf("initial # edge= %.5lf, chunksz=%.1lf, max weight dist=%.5lf \n", INITPCT_SM, CHUNKSZ_SM, w_dist_thd);
	}
	
    }
    void get_initial_edges(std::vector<unsigned int>& new_idx);
    void learn_edge_classifier(double trnsz);
    void update_new_labels(std::vector<unsigned int>& new_idx, std::vector<int>& new_lbl);
    void get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx);

    
    std::vector<unsigned int>& get_trn_idx(){ return trn_idx;};
    void get_extra_edges(std::vector<unsigned int>& ret_idx, size_t nedges);
    void compute_new_risks(std::multimap<double, unsigned int>& risks, 
			   std::map<unsigned int, double>& prop_lbl);
    
    bool is_parallel_mode(){
	return (parallel_mode==1? true : false);
    }
    
};



}
#endif