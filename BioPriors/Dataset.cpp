
#include "Dataset.h"

using namespace NeuroProof;

void Dataset::initialize(){
    
//     if (!(all_featuresu.nrows() > 0)){
// 	printf("daatset empty\n");
// 	return;
//     }
//     
//     all_featuresu.get_feature_label(all_features, all_labels); 	    	
    
    /* Debug*
     all_features.erase(all_features.begin() + 4000, all_features.end());
     all_labels.erase(all_labels.begin() + 4000, all_labels.end());
    /**/

    
    
    for(size_t ii=0; ii< all_features.size(); ii++)
	all_idx.push_back(ii);
    
      
}
std::vector< std::vector<double> >& Dataset::get_features(){
//     if (!(all_features.size()>0))
// 	all_featuresu.get_feature_label(all_features, all_labels); 	    	
    return all_features;
	
}

std::vector<int>& Dataset::get_labels(){
//     if (!(all_features.size()>0))
// 	all_featuresu.get_feature_label(all_features, all_labels); 	    	
    return all_labels;
}


void Dataset::set_trn_idx(std::vector<unsigned int>& pidx)
{
  
      
    trn_idx = pidx;
    
    tst_idx.resize(all_idx.size());
    
    std::vector<unsigned int>::iterator sit;
    
    std::vector<unsigned int> tmp_idx(trn_idx);
    sort(tmp_idx.begin(), tmp_idx.end());
    sit=set_difference(all_idx.begin(), all_idx.end(), tmp_idx.begin(), tmp_idx.end(), tst_idx.begin());
    
    tst_idx.resize(sit-tst_idx.begin());
}

void Dataset::get_train_test_data(std::vector<unsigned int>& pidx,
			     std::vector< std::vector<double> >& trnMat,
			     std::vector<int>& trnLabels,
			     std::vector< std::vector<double> >& tstMat,
			     std::vector<int>& tstLabels)
{
 
    set_trn_idx(pidx);
    
    get_submatrix(all_features, all_labels, trn_idx, trnMat, trnLabels);//experimental version
    if (trn_labels.size()>0)
	trnLabels.assign(trn_labels.begin(),trn_labels.end());
    get_submatrix(all_features, all_labels, tst_idx, tstMat, tstLabels);  
    
    
}
void Dataset::get_test_data(std::vector<unsigned int>& pidx,
			     std::vector< std::vector<double> >& tstMat,
			     std::vector<int>& tstLabels)
{
 
    set_trn_idx(pidx);
    
    get_submatrix(all_features, all_labels, tst_idx, tstMat, tstLabels);  
    
    
}
void Dataset::get_train_data(std::vector<unsigned int>& pidx,
			     std::vector< std::vector<double> >& trnMat,
			     std::vector<int>& trnLabels)
{
 
    set_trn_idx(pidx);
    
    get_submatrix(all_features, all_labels, trn_idx, trnMat, trnLabels);//experimental version
    if (trn_labels.size()>0)
	trnLabels.assign(trn_labels.begin(),trn_labels.end());
    
    
}

void Dataset::convert_idx(std::vector<unsigned int>& inidx,
			   std::vector<unsigned int>& outidx)
{
    outidx.clear();
  
    for(size_t ii=0; ii< inidx.size(); ii++){
	unsigned int tmp_idx = inidx[ii];
	outidx.push_back(tst_idx[tmp_idx]);
    }
    
    
}
void Dataset::convert_idx(unsigned int inidx, unsigned int& outidx)
{
    outidx = tst_idx[inidx];
}
void Dataset::get_submatrix(std::vector< std::vector<double> >& inputMat,
			       std::vector<int>& inputLabels,
			       std::vector<unsigned int>& ridx,
			       std::vector< std::vector<double> >& outputMat,
			       std::vector<int>& outputLabels)
{
  
    unsigned int nrows = ridx.size();
  
    outputMat.clear();
    outputLabels.clear();
    
    outputMat.resize(nrows);
    
    for(size_t ecount =0; ecount < nrows; ecount++){
	unsigned int ri = ridx[ecount];
	outputMat[ecount].insert(outputMat[ecount].begin(), inputMat[ri].begin(), inputMat[ri].end());
    } 	
    if (inputLabels.size() == inputMat.size()){
	outputLabels.resize(nrows);
	for(size_t ecount =0; ecount < nrows ; ecount++){
	    unsigned int ri = ridx[ecount];
	    outputLabels[ecount] = inputLabels[ri];
	} 	
    }
  
}





/**************************************************************************/




void Dataset::get_random_train_test_data(unsigned int nrows,
			       std::vector<unsigned int>& ridx,
			       std::vector< std::vector<double> >& trnMat,
			       std::vector<int>& trnLabels, 
			       std::vector< std::vector<double> >& tstMat,
			       std::vector<int>& tstLabels)
{
    std::srand ( unsigned ( std::time(0) ) );

    if (!(all_features.size()>0)){
	printf("Features not computed \n");
	return;
    }
    ridx.clear();
    for(size_t ii=0; ii < all_features.size(); ii++)
	ridx.push_back(ii);
    
    std::random_shuffle(ridx.begin(), ridx.end());	
    ridx.erase(ridx.begin()+nrows, ridx.end());
    
    set_trn_idx(ridx);
    
    get_submatrix(all_features, all_labels, trn_idx, trnMat, trnLabels);  
    get_submatrix(all_features, all_labels, tst_idx, tstMat, tstLabels);  
    
}

void Dataset::get_random_submatrix(std::vector< std::vector<double> >& inputMat,
			       std::vector<int>& inputLabels,
			       unsigned int nrows,
			       std::vector< std::vector<double> >& outputMat,
			       std::vector<int>& outputLabels)
{
    std::srand ( unsigned ( std::time(0) ) );
  
    std::vector<unsigned int> ridx;
    for(size_t ii=0; ii < inputMat.size(); ii++)
	ridx.push_back(ii);
    
    std::random_shuffle(ridx.begin(), ridx.end());	
    
    ridx.erase(ridx.begin()+nrows, ridx.end());
  
    get_submatrix(inputMat, inputLabels, ridx, outputMat, outputLabels);
}

void Dataset::get_train_test_set(std::vector< std::vector<double> >& inputMat,
			       std::vector<int>& inputLabels,
			       unsigned int nrows,
			       std::vector< std::vector<double> >& trnMat,
			       std::vector<int>& trnLabels, 
			       std::vector< std::vector<double> >& tstMat,
			       std::vector<int>& tstLabels,
			       std::vector<unsigned int>* ret_trn_idx,
			       std::vector<unsigned int>* ret_tst_idx)
{
    std::srand ( unsigned ( std::time(0) ) );

    std::vector<unsigned int> ridx;
    for(size_t ii=0; ii < inputMat.size(); ii++)
	ridx.push_back(ii);
    
    std::random_shuffle(ridx.begin(), ridx.end());	
    if(ret_trn_idx != NULL){
	ret_trn_idx->clear();
	ret_trn_idx->insert(ret_trn_idx->begin(), ridx.begin(), ridx.begin()+nrows);
    }
    
    std::vector<unsigned int> tst_idx;
    tst_idx.insert(tst_idx.begin(), ridx.begin()+ nrows, ridx.end());
    if(ret_tst_idx != NULL){
	ret_tst_idx->clear();
	ret_tst_idx->insert(ret_tst_idx->begin(), tst_idx.begin(), tst_idx.end());
    }
    ridx.erase(ridx.begin()+nrows, ridx.end());
  
    get_submatrix(inputMat, inputLabels, ridx, trnMat, trnLabels);
    get_submatrix(inputMat, inputLabels, tst_idx, tstMat, tstLabels);
}
