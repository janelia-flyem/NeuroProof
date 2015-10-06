

#include "IterativeLearn_iwal.h"

using namespace NeuroProof;


void IterativeLearn_iwal::get_initial_edges(std::vector<unsigned int>& new_idx){
    std::vector< std::vector<double> >& all_features = dtst.get_features();
    std::vector< int >& all_labels = dtst.get_labels();
    compute_all_edge_features(all_features, all_labels);
    dtst.initialize();
    
    std::srand ( unsigned ( std::time(0) ) );
    size_t chunksz = CHUNKSZ;
    size_t nsamples = all_features.size();
    
    size_t start_with = (size_t) (INITPCT*nsamples);

    std::srand ( unsigned ( std::time(0) ) );
    new_idx.clear();
    for(size_t ii=0; ii < all_features.size(); ii++)
	new_idx.push_back(ii);
    
    std::random_shuffle(new_idx.begin(), new_idx.end());	
    new_idx.erase(new_idx.begin()+start_with, new_idx.end());
    
//     dtst.get_train_data(new_idx, cum_train_features, cum_train_labels);
    
    
}

  



void IterativeLearn_iwal::update_new_labels(std::vector<unsigned int>& new_idx,
					    std::vector<int>& new_lbl)
{
    trn_idx.insert(trn_idx.end(), new_idx.begin(), new_idx.end());
    dtst.append_trn_labels(new_lbl);
    dtst.get_train_data(trn_idx, cum_train_features, cum_train_labels);

}
void IterativeLearn_iwal::get_bootstrap_idx(size_t len, std::vector<unsigned int>& retvec)
{
    retvec.clear();
    for (size_t ii=0; ii < len; ii++){
	unsigned int idx = (unsigned int)(rand() % len);
	retvec.push_back(idx);
    }
  
}

void IterativeLearn_iwal::get_bootstrap_trset(std::vector<unsigned int >& bootstrap_idx,
				  std::vector< std::vector<double> >& bootstrap_features,
				  std::vector< int >& bootstrap_labels)
{
    bootstrap_features.clear();
    bootstrap_labels.clear();
    for(size_t ii=0; ii < bootstrap_idx.size(); ii++){
	unsigned int idx = bootstrap_idx[ii];
	bootstrap_features.push_back(cum_train_features[idx]);
	bootstrap_labels.push_back(cum_train_labels[idx]);
    }
}

double IterativeLearn_iwal::compute_max_loss_deviance(std::vector<double>& sample)
{
    size_t nclass = 2;
    int classes[]={-1, 1};
    double max_dev = 0.0;
    
    std::vector< std::vector<double> > losses(nclass);
    for (int cc = 0; cc < nclass ; cc++){
	int label = classes[cc];
	losses[cc].resize(nclassifiers);
	for(size_t ii=0; ii<nclassifiers; ii++){
	    double predp = (clfr_pool[ii])->predict(sample);
	    predp = 2*(predp-0.5);
	    
	    double loss = fabs(1 - (predp*label));
	    
	    losses[cc][ii] = loss;
	}
    }
    
    for (int cc = 0; cc < nclass ; cc++){
	for(size_t ii=0; ii < nclassifiers; ii++)
	    for(size_t jj=0; jj < nclassifiers; jj++){
		double ldiff = fabs(losses[cc][jj] - losses[cc][ii]);
		max_dev = (max_dev < ldiff)? ldiff: max_dev;
	    }
    }
    
    return max_dev;
}

double IterativeLearn_iwal::compute_max_pred_deviance(std::vector<double>& sample)
{
    double max_dev = 0.0;
    
    std::vector< double > pred(nclassifiers);
    for(size_t ii=0; ii<nclassifiers; ii++){
	double predp = (clfr_pool[ii])->predict(sample);
	predp = 2*(predp-0.5);
	
	pred[ii] = predp;
    }
    
    for(size_t ii=0; ii < nclassifiers; ii++)
	for(size_t jj=0; jj < nclassifiers; jj++){
	    double ldiff = fabs(pred[jj] - pred[ii]);
	    max_dev = (max_dev < ldiff)? ldiff: max_dev;
	}
    
    return max_dev;
}

void IterativeLearn_iwal::get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx)
{ 
    std::vector< std::vector<double> > bootstrap_features;
    std::vector< int > bootstrap_labels;
    std::vector<unsigned int > bootstrap_idx;
    size_t ninit = cum_train_features.size();
    for(size_t ii=0; ii< nclassifiers; ii++){
	get_bootstrap_idx(ninit, bootstrap_idx);
	get_bootstrap_trset(bootstrap_idx, bootstrap_features, bootstrap_labels);
	clfr_pool.push_back(new OpencvRFclassifier);
	(clfr_pool[ii])->learn(bootstrap_features, bootstrap_labels);
    }
    
    for(size_t jj=0; jj < trn_idx.size(); jj++)
	wt_trn_idx.insert(std::make_pair(trn_idx[jj], (pmin/1.0) ));
    
    dtst.get_test_data(trn_idx, rest_features, rest_labels);  
    
    unsigned int idx2;
    for(int ecount=0;ecount< rest_features.size(); ecount++){
	double max_dev = compute_max_pred_deviance(rest_features[ecount]);//loss deviance is worse
	
	double pt = pmin + (1-pmin)*max_dev;
	prob.push_back(pt);
	
	double uniform_sample = rand()*1.0/RAND_MAX;
	if (uniform_sample < pt){
	    dtst.convert_idx(ecount,idx2);
	    wt_trn_idx.insert(std::make_pair(idx2, (pmin/pt) ));
	}
	
    }
    new_idx.clear();
    std::vector<unsigned int> tmp_idx;
    std::vector<unsigned int> add_idx;
    
    rejection_sample(feat2add, tmp_idx);
    dtst.convert_idx(tmp_idx, add_idx);
    new_idx.insert(new_idx.end(), add_idx.begin(), add_idx.end());
      
}
void IterativeLearn_iwal::rejection_sample(size_t len, std::vector<unsigned int>& add_idx )
{
    
    std::vector<unsigned int> tmp_idx;
    std::map<unsigned int, double>::iterator witr = wt_trn_idx.begin();
    double cmax = 0.0;
    for(; witr != wt_trn_idx.end(); witr++){
	unsigned int idx = witr->first;
	double cost = witr->second;
	tmp_idx.push_back(idx);
	
	cmax = (cmax < cost)? cost: cmax;
    }
    size_t count=0, iter = 0;
    size_t dlen = tmp_idx.size();
    while (count < len && iter < 10000){
	unsigned int random_loc = (unsigned int)(rand() % dlen);
	unsigned int idx = tmp_idx[random_loc];
	
	double uniform_sample = rand()*1.0/RAND_MAX;
	double acceptance_thd = wt_trn_idx[idx];
	acceptance_thd /= cmax;
	
	if (uniform_sample < acceptance_thd){
	    add_idx.push_back(idx);
	    count++;
	}

	iter++;
    }
}
void IterativeLearn_iwal::learn_edge_classifier(double trnsz)
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
    
    size_t st=0;
    size_t multiple_IVAL=1;
    
    get_next_edge_set(trnsz, new_idx);
    trn_idx.clear();
    dtst.clear_trn_labels();
    
    new_lbl.clear();
    new_lbl.resize(new_idx.size());
    for(size_t ii=0; ii < new_idx.size() ; ii++){
	unsigned int idx = new_idx[ii];
	new_lbl[ii] = all_labels[idx];
    }
    update_new_labels(new_idx, new_lbl);
    feature_mgr->get_classifier()->learn(cum_train_features, cum_train_labels); // number of trees

    double thd_u = 0.3;
//     do{
//       
// 	
// 	std::vector<unsigned int> new_idx;
// 	//std::vector<unsigned int> certain_idx;
// 	get_next_edge_set(CHUNKSZ, new_idx);
// 	
// 	printf("rf accuracy\n");
// 	//dtst.get_test_data(trn_idx, rest_features, rest_labels);
// 	evaluate_accuracy(rest_features,rest_labels, thd_u);
// 
// 	if((clfr_name.size()>0) && cum_train_features.size()>(multiple_IVAL*IVAL)){
// 	    update_clfr_name(clfr_name, multiple_IVAL*IVAL);
// 	    feature_mgr->get_classifier()->save_classifier(clfr_name.c_str());  
// 	    multiple_IVAL++;
// 	}
// 	
// 	new_lbl.clear();
// 	std::vector<int> new_lbl(new_idx.size());
// 	for(size_t ii=0; ii < new_idx.size() ; ii++){
// 	    unsigned int idx = new_idx[ii];
// 	    new_lbl[ii] = all_labels[idx];
// 	}
// 	
// 	update_new_labels(new_idx,new_lbl);
// 	
// 	st++;
// 	
//     }while(st<nitr);
    
    dtst.get_test_data(trn_idx, rest_features, rest_labels);  
    evaluate_accuracy(rest_features,rest_labels, thd_u);
    double npos=0, nneg=0;
    for(size_t ii=0; ii<cum_train_labels.size(); ii++){
	npos += (cum_train_labels[ii]>0?1:0);
	nneg += (cum_train_labels[ii]<0?1:0);
    }

    printf("number of positive : %.1lf, negative: %.1lf\n",npos,nneg);
    
}



