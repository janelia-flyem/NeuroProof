

#include "IterativeLearn_simulate.h"

using namespace NeuroProof;



void IterativeLearn_simulate::get_initial_edges(std::vector<unsigned int>& new_idx){

    std::vector< std::vector<double> >& all_features = dtst.get_features();
    std::vector< int >& all_labels = dtst.get_labels();
    compute_all_edge_features(all_features, all_labels);
    dtst.initialize();
    
    printf("total edges generated: %u\n",all_features.size());
    
    std::srand ( unsigned ( std::time(0) ) );
    size_t start_with = 500;

    // *C* get initial samples
    std::vector<unsigned int> tmp_idx;
    tmp_idx.clear();
    for(size_t ii=0; ii < all_features.size(); ii++)
	tmp_idx.push_back(ii);
    
    std::random_shuffle(tmp_idx.begin(), tmp_idx.end());	
    tmp_idx.erase(tmp_idx.begin()+ start_with, tmp_idx.end());
    new_idx = tmp_idx;
    
//     std::vector< int > tmp_lbl(tmp_idx.size());
//     for(size_t ii=0; ii < tmp_idx.size() ; ii++){
// 	unsigned int idx = tmp_idx[ii];
// 	tmp_lbl[ii] = all_labels[idx];
//     }
//     
//     // *C* learn initial classifier
//     dtst.append_trn_labels(tmp_lbl);
//     std::vector< std::vector<double> > tmp_train_features;
//     std::vector< int > tmp_train_labels;
//     dtst.get_train_data(tmp_idx, tmp_train_features, tmp_train_labels);
//     
//     feature_mgr->get_classifier()->learn(tmp_train_features, tmp_train_labels); // number of trees
//     
//     std::vector< std::vector<double> > tmp_rest_features;
//     std::vector< int > tmp_rest_labels;
//     dtst.get_test_data(tmp_idx, tmp_rest_features, tmp_rest_labels);  
//     
//     // *C* compute prediction - label agreement to find the most obvious one.
//     std::multimap<double, unsigned int> init_risks;
//     for(size_t ii=0; ii < tmp_rest_features.size(); ii++){
//     
// 	double rf_p1 = feature_mgr->get_classifier()->predict(tmp_rest_features[ii]);
// 	double rf_p = 2*(rf_p1-0.5);
// 	
// 	double risk = rf_p*tmp_rest_labels[ii];
// 	
// 	unsigned int tidx;
// 	dtst.convert_idx(ii, tidx);
// 	
// 	init_risks.insert(std::make_pair(risk, tidx));
//     }
// 
//     // *C* the indices of most obvious ones
//     new_idx.clear();
//     std::multimap<double, unsigned int>::reverse_iterator rit = init_risks.rbegin();
//     size_t count = 0, npos = 0, nneg = 0;
//     while((count<start_with) && (rit!=init_risks.rend())){
// 	double risk = rit->first;
// 	unsigned int idx = rit->second;
// 	
// 	++rit;
// 	
// 	
// 	if (nneg>(0.65*start_with) && (all_labels[idx] == -1))
// 	    continue;
// 	if (npos>(0.65*start_with) && (all_labels[idx] == 1))
// 	    continue;
// 	
// 	(all_labels[idx] == -1)? nneg++ : npos++;
// 	count++;
// 	new_idx.push_back(idx);
//     }
//     
//     dtst.clear_trn_labels();
//     dtst.initialize();
//     std::time(&end);	
//     printf("Time needed to compute W : %.2f min\n", (difftime(end,start))*1.0/60);
    
        
}

void IterativeLearn_simulate::compute_new_risks(std::multimap<double, unsigned int>& risks,
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

void IterativeLearn_simulate::get_next_edge_set(size_t feat2add, std::vector<unsigned int>& new_idx)
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



void IterativeLearn_simulate::update_new_labels(std::vector<unsigned int>& new_idx,
					    std::vector<int>& new_lbl)
{
  
    trn_idx.insert(trn_idx.end(), new_idx.begin(), new_idx.end());
    dtst.append_trn_labels(new_lbl);
    dtst.get_train_data(trn_idx, cum_train_features, cum_train_labels);

// //     std::time_t start, end;
// //     std::time(&start);	
// //     wt1->add2trnset(trn_idx, cum_train_labels);
// // //     threadp = new boost::thread(&WeightMatrix1::add2trnset, wt1, trn_idx, cum_train_labels);
// //     std::time(&end);	
// //     printf("C: Time to update: %.2f sec\n", (difftime(end,start))*1.0);
    
}


void IterativeLearn_simulate::get_extra_edges(std::vector<unsigned int>& ret_idx, size_t nedges)
{
  
    ret_idx.clear();
    
    if(init_trn_idx.size() > nedges)
	ret_idx.insert(ret_idx.begin(), init_trn_idx.begin(), init_trn_idx.begin()+nedges);
    else
	ret_idx.insert(ret_idx.begin(), init_trn_idx.begin(), init_trn_idx.end());
    
}


void IterativeLearn_simulate::learn_edge_classifier(double trnsz){
    
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
    size_t feat2add = CHUNKSZ_SM;
    size_t nitr = (size_t) (remaining/feat2add);
    size_t st=0;
    size_t multiple_IVAL=1;
    //string clfr_name = pclfr_name; 
    double thd_s = 0.3;
    
    std::vector<unsigned int> rest_idx;
    unsigned int iidx;
    do{
      
	feature_mgr->get_classifier()->learn(cum_train_features, cum_train_labels); // number of trees
	new_idx.clear();
	rest_idx.clear();
	
	dtst.get_test_data(trn_idx, rest_features, rest_labels);  
	
	std::multimap<double, unsigned int> new_risks;
	for(size_t ii=0; ii < rest_features.size(); ii++){
	
	    double rf_p1 = feature_mgr->get_classifier()->predict(rest_features[ii]);
	    double rf_p = 2*(rf_p1-0.5);
	    
	    double risk = rf_p*rest_labels[ii];
	    
	    dtst.convert_idx(ii,iidx);
	    if(ii!=iidx)
	      int tmppp=1;
	    
	    new_risks.insert(std::make_pair(risk, iidx));
	}
	
// 	dtst.convert_idx(rest_idx, new_idx);
	
	// *C* any misclassified sample is equally likely to be picked
// 	std::random_shuffle(new_idx.begin(), new_idx.end());	
// 	new_idx.erase(new_idx.begin()+ feat2add, new_idx.end());
	std::multimap<double, unsigned int>::iterator fit = new_risks.begin();
	size_t count = 0, npos = 0, nneg = 0;
	
	while( (count<feat2add) && (fit!=new_risks.end())){
	    double risk = fit->first;
	    unsigned int idx = fit->second;
	    
	    ++fit;
	    count++;
	    
	    (all_labels[idx] == -1)? nneg++ : npos++;
	    
	    new_idx.push_back(idx);
	}
	
	new_lbl.clear();
	for(size_t ii=0; ii < feat2add ; ii++){
	    size_t idx = new_idx[ii];
	    new_lbl.push_back(all_labels[idx]);
	}
	
	
	update_new_labels(new_idx, new_lbl);
	
	st++;

    }while(st<nitr);
    
    // *C* save the trn indices
    int rand_num = rand() % 5000 + 1;
    char rand_num_str[1024];
    //itoa(rand_num, rand_num_str.c_str(),10);
    sprintf(rand_num_str, "%d" , rand_num);
    string trn_idx_savename = "trn_idx_simulate_";
    trn_idx_savename += rand_num_str;
    trn_idx_savename += ".txt";
    
    FILE* fp = fopen(trn_idx_savename.c_str(), "wt");
    for(size_t ii=0; ii<trn_idx.size(); ii++ )
	fprintf(fp, "%u\n", trn_idx[ii]);
    fclose(fp);
    
    
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
