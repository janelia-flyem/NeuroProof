#ifndef _DATASET
#define _DATASET

#include "../Utilities/unique_row_matrix.h"
#include <vector>
#include <algorithm>
#include <ctime>


namespace NeuroProof{

class Dataset{
  
    UniqueRowFeature_Label all_featuresu;
    std::vector<int> all_labels;  
    std::vector<int> trn_labels;  
    std::vector< std::vector<double> > all_features;
    std::vector<unsigned int> trn_idx;
    std::vector<unsigned int> tst_idx;
    std::vector<unsigned int> all_idx;
    
public:
    Dataset(){
	trn_idx.clear(); all_idx.clear(); tst_idx.clear();
	trn_labels.clear();
    };
    void initialize();
    UniqueRowFeature_Label& get_unique_features(){return all_featuresu;};
    void set_trn_idx(std::vector<unsigned int>& pidx);
    std::vector< std::vector<double> >& get_features();
    std::vector<int>& get_labels();
    
    std::vector<unsigned int>&  get_trn_idx(){return trn_idx;};
    void set_trn_labels(std::vector<int>& plabels){
	trn_labels =  plabels;
    }
    void append_trn_labels(std::vector<int>& plabels){
	trn_labels.insert(trn_labels.end(), plabels.begin(), plabels.end());
    }
    void clear_trn_labels(){
	trn_labels.clear();
    }
    
    void get_train_test_data(std::vector<unsigned int>& pidx,
			     std::vector< std::vector<double> >& trnMat,
			     std::vector<int>& trnLabels,
			     std::vector< std::vector<double> >& tstMat,
			     std::vector<int>& tstLabels);
    void get_train_data(std::vector<unsigned int>& pidx,
			     std::vector< std::vector<double> >& trnMat,
			     std::vector<int>& trnLabels);
    void get_test_data(std::vector<unsigned int>& pidx,
			     std::vector< std::vector<double> >& tstMat,
			     std::vector<int>& tstLabels);
    void get_random_train_test_data(unsigned int nrows,
				std::vector<unsigned int>& ridx,
				std::vector< std::vector<double> >& trnMat,
			       std::vector<int>& trnLabels, 
			       std::vector< std::vector<double> >& tstMat,
			       std::vector<int>& tstLabels);
    void convert_idx(std::vector<unsigned int>& inidx,
			   std::vector<unsigned int>& outidx);
    
    void convert_idx(unsigned int inidx, unsigned int& outidx);
    
    
    
    
    
    
    
    
    void get_submatrix(std::vector< std::vector<double> >& inputMat,
			       std::vector<int>& inputLabels,
			       std::vector<unsigned int>& ridx,
			       std::vector< std::vector<double> >& outputMat,
			       std::vector<int>& outputLabels);

    void get_random_submatrix(std::vector< std::vector<double> >& inputMat,
			       std::vector<int>& inputLabels,
			       unsigned int nrows,
			       std::vector< std::vector<double> >& outputMat,
			       std::vector<int>& outputLabels);

    void get_train_test_set(std::vector< std::vector<double> >& inputMat,
			       std::vector<int>& inputLabels,
			       unsigned int nrows,
			       std::vector< std::vector<double> >& trnMat,
			       std::vector<int>& trnLabels, 
			       std::vector< std::vector<double> >& tstMat,
			       std::vector<int>& tstLabels,
			       std::vector<unsigned int>* ret_trn_idx = NULL,
			       std::vector<unsigned int>* ret_tst_idx = NULL);
};

}

#endif