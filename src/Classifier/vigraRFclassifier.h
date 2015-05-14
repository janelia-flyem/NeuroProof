
#ifndef _vigra_rf_classifier
#define _vigra_rf_classifier

#include <vigra/multi_array.hxx>
#include <vigra/random_forest.hxx>
#include <vigra/random_forest/rf_common.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/random_forest_hdf5_impex.hxx>

#include "edgeclassifier.h"

using namespace std;
using namespace vigra;
using namespace rf;

typedef MultiArray<2, double>::difference_type Shape;

class VigraRFclassifier: public EdgeClassifier{


     RandomForest<>* _rf;
     int _nfeatures;
     int _nclass;	
	
    std::vector<unsigned int> ignore_featlist;

public:
     VigraRFclassifier():_rf(NULL){};	
     VigraRFclassifier(const char* rf_filename);
     ~VigraRFclassifier(){
	 if (_rf) delete _rf;
     }	
     void  load_classifier(const char* rf_filename);
     double predict(std::vector<double>& features);
     void learn(std::vector< std::vector<double> >& pfeatures, std::vector<int>& plabels);
     void save_classifier(const char* rf_filename);

     void set_ignore_featlist(std::vector<unsigned int>& pignore_list){ignore_featlist = pignore_list;};
     void get_ignore_featlist(std::vector<unsigned int>& pignore_list){pignore_list = ignore_featlist;};
     
     
     bool is_trained(){
	if (_rf && _rf->tree_count()>0)
	   return true;
	else 
	   return false;
		
     };

};

#endif
