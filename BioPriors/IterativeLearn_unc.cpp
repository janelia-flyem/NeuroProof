

#include "IterativeLearn_unc.h"

using namespace NeuroProof;


void IterativeLearn_uncertain::get_initial_edges(std::vector<unsigned int>& new_idx){
    std::vector< std::vector<double> >& all_features = dtst.get_features();
    std::vector< int >& all_labels = dtst.get_labels();
    compute_all_edge_features(all_features, all_labels);
    dtst.initialize();
    
    std::srand ( unsigned ( std::time(0) ) );
    size_t chunksz = CHUNKSZ;
    size_t nsamples = all_features.size();
    
    size_t start_with = (size_t) (INITPCT*nsamples);
    start_with = (start_with > 100) ? 100: start_with;

    std::srand ( unsigned ( std::time(0) ) );
    new_idx.clear();
    for(size_t ii=0; ii < all_features.size(); ii++)
	new_idx.push_back(ii);
    
    std::random_shuffle(new_idx.begin(), new_idx.end());	
    new_idx.erase(new_idx.begin()+start_with, new_idx.end());
    
//     dtst.get_train_data(new_idx, cum_train_features, cum_train_labels);
    
}


void IterativeLearn_uncertain::update_new_labels(std::vector<unsigned int>& new_idx,
					    std::vector<int>& new_lbl)
{
    trn_idx.insert(trn_idx.end(), new_idx.begin(), new_idx.end());
    dtst.append_trn_labels(new_lbl);
    dtst.get_train_data(trn_idx, cum_train_features, cum_train_labels);

}



void IterativeLearn_uncertain::get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx)
{ 
    feature_mgr->get_classifier()->learn(cum_train_features, cum_train_labels); // number of trees
    dtst.get_test_data(trn_idx, rest_features, rest_labels);  
    std::vector<unsigned int> uncertain_idx;
    for(int ecount=0;ecount< rest_features.size(); ecount++){
	double predp = feature_mgr->get_classifier()->predict(rest_features[ecount]);
	int predl = (predp>0.5)? 1:-1;	
	
	if ((predp>0.35) && (predp<0.65))
	    uncertain_idx.push_back(ecount);
// 	if ((predp<0.15) || (predp>0.85))
// 	    certain_idx.push_back(ecount);
	
    }
    std::random_shuffle(uncertain_idx.begin(), uncertain_idx.end());
    size_t nretain = (feat2add < uncertain_idx.size()? feat2add : uncertain_idx.size());
    uncertain_idx.erase(uncertain_idx.begin()+nretain, uncertain_idx.end());
    
    dtst.convert_idx(uncertain_idx, new_idx);
}

void IterativeLearn_uncertain::learn_edge_classifier(double trnsz)
{
    
    std::vector<unsigned int> new_idx;

    get_initial_edges(new_idx);
    std::vector<int> new_lbl(new_idx.size());
    std::vector< int >& all_labels = dtst.get_labels();
    for(size_t ii=0; ii < new_idx.size() ; ii++){
	unsigned int idx = new_idx[ii];
	new_lbl[ii] = all_labels[idx];
    }
    update_new_labels(new_idx, new_lbl);
    

    double remaining = trnsz - cum_train_features.size();
    
    size_t nitr = (size_t) (remaining/CHUNKSZ);
    size_t st=0;
    size_t multiple_IVAL=1;

    double thd_u = 0.3;
    do{
      
	
	std::vector<unsigned int> new_idx;
	//std::vector<unsigned int> certain_idx;
	get_next_edge_set(CHUNKSZ, new_idx);
	
	printf("rf accuracy\n");
	//dtst.get_test_data(trn_idx, rest_features, rest_labels);
	evaluate_accuracy(rest_features,rest_labels, thd_u);

	if((clfr_name.size()>0) && cum_train_features.size()>(multiple_IVAL*IVAL)){
	    update_clfr_name(clfr_name, multiple_IVAL*IVAL);
	    feature_mgr->get_classifier()->save_classifier(clfr_name.c_str());  
	    multiple_IVAL++;
	}
	
	new_lbl.clear();
	std::vector<int> new_lbl(new_idx.size());
	for(size_t ii=0; ii < new_idx.size() ; ii++){
	    unsigned int idx = new_idx[ii];
	    new_lbl[ii] = all_labels[idx];
	}
	
	update_new_labels(new_idx,new_lbl);
	
	st++;
	
    }while(st<nitr);
    
    evaluate_accuracy(rest_features,rest_labels, thd_u);
    double npos=0, nneg=0;
    for(size_t ii=0; ii<cum_train_labels.size(); ii++){
	npos += (cum_train_labels[ii]>0?1:0);
	nneg += (cum_train_labels[ii]<0?1:0);
    }

    printf("number of positive : %.1lf, negative: %.1lf\n",npos,nneg);
    
}



