

#include "IterativeLearn_semi.h"

using namespace NeuroProof;



void IterativeLearn_semi::get_initial_edges(std::vector<unsigned int>& new_idx){

    std::vector< std::vector<double> >& all_features = dtst.get_features();
    std::vector< int >& all_labels = dtst.get_labels();
    compute_all_edge_features(all_features, all_labels);
    dtst.initialize();
    
    feature_mgr->find_useless_features(all_features);
    
    printf("total edges generated: %u\n",all_features.size());
    printf("total features: %u\n",all_features[0].size());
    
    std::srand ( unsigned ( std::time(0) ) );

    
     
    size_t start_with = (size_t) (INITPCT_SM*all_features.size());
    
    std::time_t start, end;
    std::time(&start);

    /*C* Assuming feature format: node 1 feat, node 2 feat, edges feat and diff feat
     * The first feature for each of these three components is size which we ignore
     * followed by 4 moments and 5 percentiles for each channel
     /**/ 
    
//     unsigned int tmp_ignore[] = {0, 55, 110, 165}; 
//     unsigned int tmp_ignore[4];
//     tmp_ignore[0] = 0;
//     for(size_t ff=1 ;ff<4; ff++)
// 	tmp_ignore[ff] = tmp_ignore[ff-1]+ (1 + nfeat_channels*4 + nfeat_channels *5); 
//     
//     printf("ignore features:");
//     for(size_t ff=0 ;ff<4; ff++)
// 	printf("%u ", tmp_ignore[ff]);
//     printf("\n");
      
//     printf("sample values of ignore features:");
//     for(size_t ff=0 ;ff<4; ff++)
// 	printf("%lf ", all_features[0][tmp_ignore[ff]]);
//     printf("\n");
//     
//     printf("sample values of all features:");
//     for(size_t ff=0 ;ff<15; ff++)
// 	printf("%lf ", all_features[0][ff]);
//     printf("\n");
    
//     std::vector<unsigned int> ignore_list(tmp_ignore, tmp_ignore + sizeof(tmp_ignore)/sizeof(unsigned int));
   
    std::vector<unsigned int> ignore_list;

    wt1 = new WeightMatrix1(w_dist_thd, ignore_list);
    wt1->weight_matrix_parallel(all_features, false);
    
    
    
    std::time(&end);	
    printf("Time needed to compute W : %.2f min\n", (difftime(end,start))*1.0/60);
    
        
    if (initial_set_strategy == DEGREE)
	//*C* initial samples by large degree
	wt1->find_large_degree(init_trn_idx);
    else if  (initial_set_strategy == KMEANS){ 
	//*C* initial samples by clustering, e.g., kmeans.
	std::vector< std::vector<double> > scaled_features;
	wt1->scale_features(all_features, scaled_features);
	
	kMeans km(start_with, 100, 1e-2);
	km.compute_centers(scaled_features, init_trn_idx);
    }
    
    new_idx = init_trn_idx;
    if (new_idx.size()>start_with)
	new_idx.erase(new_idx.begin()+ start_with, new_idx.end());
    
    if (init_trn_idx.size()> start_with)
	init_trn_idx.erase(init_trn_idx.begin(), init_trn_idx.begin()+start_with);
//     else
// 	init_trn_idx.erase(init_trn_idx.begin(), init_trn_idx.end());
}

void IterativeLearn_semi::compute_new_risks(std::multimap<double, unsigned int>& risks,
					    std::map<unsigned int, double>& prop_lbl)
{

    feature_mgr->get_classifier()->learn(cum_train_features, cum_train_labels); // number of trees


//     threadp->join();
    std::time_t start, end;
    std::time(&start);	
    wt1->AMGsolve(prop_lbl);
    std::time(&end);	
    printf("C: Time to solve linear equations: %.2f sec\n", (difftime(end,start))*1.0);
//     delete threadp;
    
    std::vector< std::vector<double> >& all_features = dtst.get_features();

    double prop_diff=0;


    unsigned int idx;
    double pp;
	
    std::map<unsigned int, double>::iterator plit;
    for(plit = prop_lbl.begin(); plit != prop_lbl.end(); plit++){
	
	idx = plit->first;
	pp = plit->second;
	
	double pp_clipped = (pp > 1.0)? 1.0 : pp;
	pp_clipped = (pp_clipped < -1.0)? -1.0: pp;
	
	
	double rf_p1 = feature_mgr->get_classifier()->predict(all_features[idx]);
	double rf_p = 2*(rf_p1-0.5);

	
	
	double risk = rf_p * pp_clipped ;
	
	risks.insert(std::make_pair(risk, idx));
    }
}

void IterativeLearn_semi::get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx)
{
    new_idx.clear();
    
    std::multimap<double, unsigned int> risks;
    std::map<unsigned int, double> prop_lbl;
    compute_new_risks(risks, prop_lbl);
    
    std::multimap<double, unsigned int>::iterator rit = risks.begin();
    for(size_t ii=0; ii < feat2add && rit != risks.end() ; ii++, rit++){
	unsigned int idx = rit->second;
	new_idx.push_back(idx);
    }
    
}



void IterativeLearn_semi::update_new_labels(std::vector<unsigned int>& new_idx,
					    std::vector<int>& new_lbl)
{
  
    trn_idx.insert(trn_idx.end(), new_idx.begin(), new_idx.end());
    dtst.append_trn_labels(new_lbl);
    dtst.get_train_data(trn_idx, cum_train_features, cum_train_labels);

    std::time_t start, end;
    std::time(&start);	
    wt1->add2trnset(trn_idx, cum_train_labels);
//     threadp = new boost::thread(&WeightMatrix1::add2trnset, wt1, trn_idx, cum_train_labels);
    std::time(&end);	
    printf("C: Time to update: %.2f sec\n", (difftime(end,start))*1.0);
    
}


void IterativeLearn_semi::get_extra_edges(std::vector<unsigned int>& ret_idx, size_t nedges)
{
  
    ret_idx.clear();
    
    if(init_trn_idx.size() > nedges)
	ret_idx.insert(ret_idx.begin(), init_trn_idx.begin(), init_trn_idx.begin()+nedges);
    else
	ret_idx.insert(ret_idx.begin(), init_trn_idx.begin(), init_trn_idx.end());
    
}


void IterativeLearn_semi::learn_edge_classifier(double trnsz){
    
    std::vector<unsigned int> new_idx;

    get_initial_edges(new_idx);
//     std::vector< std::pair<Node_t, Node_t> > tmpedgebuffer;
//     edgelist_from_index(new_idx, tmpedgebuffer);
    
    std::vector<int> new_lbl(new_idx.size());
    std::vector< int >& all_labels = dtst.get_labels();
    for(size_t ii=0; ii < new_idx.size() ; ii++){
	unsigned int idx = new_idx[ii];
	new_lbl[ii] = all_labels[idx];
    }
    update_new_labels(new_idx, new_lbl);
    
    // ***********//  
    
    
    double remaining = trnsz - trn_idx.size();
    std::vector< std::vector<double> >& all_features = dtst.get_features();
    
    std::time_t start, end;
    size_t chunksz = CHUNKSZ_SM;
    size_t nitr = (size_t) (remaining/chunksz);
    size_t st=0;
    size_t multiple_IVAL=1;
    //string clfr_name = pclfr_name; 
    double thd_s = 0.3;
    
    do{
      
	std::multimap<double, unsigned int> risks;
        std::map<unsigned int, double> prop_lbl;
	compute_new_risks(risks, prop_lbl);
	
	/*Debug*/
	printf("rf accuracy\n");
	dtst.get_test_data(trn_idx, rest_features, rest_labels);
	evaluate_accuracy(rest_features,rest_labels, 0.3);
	printf("nn accuracy\n");
	std::vector<int> tmp_lbl;
	std::vector<double> tmp_pred;
	std::map<unsigned int, double>::iterator plit;
	for(plit = prop_lbl.begin(); plit != prop_lbl.end(); plit++){
	    unsigned int idx = plit->first;
	    double pp = plit->second;
	    tmp_lbl.push_back(all_labels[idx]);
	    tmp_pred.push_back(pp);
	}
	evaluate_accuracy(tmp_lbl,tmp_pred, 0.0);
      
	/**/
	if((clfr_name.size()>0) && cum_train_features.size()>(multiple_IVAL*IVAL)){
	    update_clfr_name(clfr_name, multiple_IVAL*IVAL);
	    feature_mgr->get_classifier()->save_classifier(clfr_name.c_str());  
	    multiple_IVAL++;
	}
      
	new_idx.clear();
	new_lbl.clear();
	
	
	
	unsigned int feat2add = chunksz;
	unsigned int npos = 0 , nneg = 0;
	unsigned int rf_incorrect=0;
	unsigned int nn_incorrect=0;
// 	std::multimap<double, unsigned int>::reverse_iterator rit = risks.rbegin();
// 	for(size_t ii=0; ii < feat2add && rit != risks.rend() ; ii++, rit++){
	std::multimap<double, unsigned int>::iterator rit = risks.begin();
	for(size_t ii=0; ii < feat2add && rit != risks.end() ; ii++, rit++){
	    unsigned int idx = rit->second;
	    new_idx.push_back(idx);
	    
	    new_lbl.push_back(all_labels[idx]);
	    if (all_labels[idx] > 0) 
	      npos++;
	    else 
	      nneg++;
	    
	    /*debug*/
	    int lbl1 = all_labels[idx];
	    double pp_clipped = prop_lbl[idx];
	    double rf_p1 = feature_mgr->get_classifier()->predict(all_features[idx]);
	    double rf_p = 2*(rf_p1-0.5);
	    
	    
	    if ((lbl1>0) && (rf_p<0) && (pp_clipped>0))
		rf_incorrect++;
	    else if ((lbl1<0) && (rf_p>0) && (pp_clipped<0))
		rf_incorrect++;
	    
	    if ((lbl1>0) && (rf_p>0) && (pp_clipped<0))
		nn_incorrect++;
	    else if ((lbl1<0) && (rf_p<0) && (pp_clipped>0))
		nn_incorrect++;
	    /**/
	    
	    
	}
	printf("Added samples, pos: %u, neg:%u\n",npos, nneg);
	printf("Added--rf incorrect: %u, nn incorrect:%u\n",rf_incorrect, nn_incorrect);
	
	update_new_labels(new_idx, new_lbl);
	
	st++;

    }while(st<nitr);
    
    
    dtst.get_test_data(trn_idx, rest_features, rest_labels);
    evaluate_accuracy(rest_features,rest_labels, thd_s);
    double npos=0, nneg=0;
    for(size_t ii=0; ii<cum_train_labels.size(); ii++){
	npos += (cum_train_labels[ii]>0?1:0);
	nneg += (cum_train_labels[ii]<0?1:0);
    }

    printf("number of positive : %.1lf, negative: %.1lf\n",npos,nneg);
    
    /*C* debug
    fp= fopen("selected_trnidxC.txt","wt");
    for(size_t ii=0; ii < trn_idx.size(); ii++)
	fprintf(fp,"%u\n",trn_idx[ii]);
    fclose(fp);
    
    //*C*/
    
    
}
