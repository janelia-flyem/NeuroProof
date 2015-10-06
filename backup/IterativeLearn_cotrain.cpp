

#include "IterativeLearn_cotrain.h"

using namespace NeuroProof;


void IterativeLearn_co::get_initial_edges(std::vector<unsigned int>& new_idx){
    std::vector< std::vector<double> >& all_features = dtst.get_features();
    std::vector< int >& all_labels = dtst.get_labels();
    compute_all_edge_features(all_features, all_labels);
    dtst.initialize();
    
    std::srand ( unsigned ( std::time(0) ) );
    size_t chunksz = CHUNKSZ;
    size_t nsamples = all_features.size();
    
    size_t start_with = (size_t) (INITPCT*nsamples);
    start_with = start_with > 200 ? 200: start_with;

    std::srand ( unsigned ( std::time(0) ) );
    new_idx.clear();
    for(size_t ii=0; ii < all_features.size(); ii++)
	new_idx.push_back(ii);
    
    std::random_shuffle(new_idx.begin(), new_idx.end());	
    new_idx.erase(new_idx.begin()+start_with, new_idx.end());
    
//     dtst.get_train_data(new_idx, cum_train_features, cum_train_labels);
    
    
}

  



void IterativeLearn_co::update_new_labels(std::vector<unsigned int>& new_idx,
					    std::vector<int>& new_lbl)
{
    trn_idx.insert(trn_idx.end(), new_idx.begin(), new_idx.end());
    trn_lbl.insert(trn_lbl.end(), new_lbl.begin(), new_lbl.end());
//     dtst.append_trn_labels(new_lbl);
//     dtst.get_train_data(trn_idx, cum_train_features, cum_train_labels);

}

double IterativeLearn_co::compute_max_loss_deviance(std::vector<double>& sample)
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

double IterativeLearn_co::compute_max_pred_deviance(std::vector<double>& sample)
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
void IterativeLearn_co::compute_new_risks()
{
      
    dtst.get_test_data(trn_idx, rest_features, rest_labels);
    for(size_t ii=0; ii< rest_features.size(); ii++){
	double joint_blf = 1;
	for(size_t cc=0; cc < nclassifiers; cc++){
	    double predp = (clfr_pool[cc])->predict(rest_features[ii]);
	    predp = 2*(predp-0.5);
	    joint_blf *= predp;
	}  
	risks.insert(std::make_pair(joint_blf, ii));
    }
}

void IterativeLearn_co::get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx)
{ 
//     std::vector< std::vector<double> > bootstrap_features;
//     std::vector< int > bootstrap_labels;
//     std::vector<unsigned int > bootstrap_idx;
//     size_t ninit = cum_train_features.size();
    for(size_t ii=0; ii< nclassifiers; ii++){
// 	get_bootstrap_idx(ninit, bootstrap_idx);
// 	get_bootstrap_trset(bootstrap_idx, bootstrap_features, bootstrap_labels);
// 	clfr_pool.push_back(new OpencvRFclassifier);
	dtst.set_trn_labels(indiv_trn_lbl[ii]);
	dtst.get_train_data(indiv_trn_idx[ii], cum_train_features, cum_train_labels);
	(clfr_pool[ii])->learn(cum_train_features, cum_train_labels);
    }
    
    compute_new_risks();
    
    std::map<double, unsigned int>::iterator witr = risks.begin();
    std::vector<unsigned int> tmp_idx;
    for(size_t ii=0; ii< feat2add; ii++, witr++){
	unsigned int idx1 = witr->second;
	
	tmp_idx.push_back(idx1);
	
    }
    dtst.convert_idx(tmp_idx, new_idx);
//     new_idx.insert(new_idx.end(), add_idx.begin(), add_idx.end());
      
}

void IterativeLearn_co::update_indiv_trnset(std::vector<unsigned int>& new_idx,
					    std::vector<int>& new_lbl)
{
    std::vector< std::vector<double> >& all_features = dtst.get_features();
    unsigned int idx1, max_cc = 0;
    double lbldiff, max_lbldiff;
    for(size_t ii=0; ii< new_idx.size(); ii++){
	idx1 = new_idx[ii];
	max_lbldiff = 0;
	for(size_t cc=0; cc < nclassifiers; cc++){
	    double predp = (clfr_pool[cc])->predict(all_features[idx1]);
	    predp = 2*(predp-0.5);
	    lbldiff = fabs(predp - new_lbl[ii]);
	    if (lbldiff>max_lbldiff){
		max_lbldiff = lbldiff;
		max_cc = cc;
	    }
	}
	    
	indiv_trn_idx[max_cc].push_back(idx1);
	indiv_trn_lbl[max_cc].push_back(new_lbl[ii]);
    }
    
}
void IterativeLearn_co::learn_edge_classifier(double trnsz)
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
    trn_idx = new_idx;
    trn_lbl = new_lbl;
    
    int ninit_trn_feat = (int)(new_idx.size()*1.0/nclassifiers);
    for(size_t ii=0; ii<nclassifiers; ii++){
	size_t starti = ii*ninit_trn_feat;
	size_t endi = (ii+1)*ninit_trn_feat;
	(indiv_trn_idx[ii]).insert((indiv_trn_idx[ii]).end(), new_idx.begin()+starti, new_idx.begin()+endi);
      
	(indiv_trn_lbl[ii]).insert((indiv_trn_lbl[ii]).end(), new_lbl.begin()+ starti, new_lbl.begin()+endi);
    }
      
      
    double remaining = trnsz - trn_idx.size();
    
    size_t st=0;
    size_t multiple_IVAL=1;
    size_t nitr = (size_t) (remaining/CHUNKSZ);
    

    double thd_u = 0.3;
    do{
      
	printf("itr : %u\n", st+1);  
	std::vector<unsigned int> new_idx;
	//std::vector<unsigned int> certain_idx;
	get_next_edge_set(CHUNKSZ, new_idx);
	
	printf("rf accuracy\n");
	//dtst.get_test_data(trn_idx, rest_features, rest_labels);
// 	evaluate_accuracy(rest_features,rest_labels, thd_u);

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
	
	update_indiv_trnset(new_idx, new_lbl);
	
	st++;
	
    }while(st<nitr);
    
    dtst.set_trn_labels(trn_lbl);
    dtst.get_train_data(trn_idx, cum_train_features, cum_train_labels);  
    feature_mgr->get_classifier()->learn(cum_train_features, cum_train_labels);
    dtst.get_test_data(trn_idx, rest_features, rest_labels);  
    evaluate_accuracy(rest_features,rest_labels, thd_u);
    double npos=0, nneg=0;
    for(size_t ii=0; ii<cum_train_labels.size(); ii++){
	npos += (cum_train_labels[ii]>0?1:0);
	nneg += (cum_train_labels[ii]<0?1:0);
    }

    printf("number of positive : %.1lf, negative: %.1lf\n",npos,nneg);
    
}



