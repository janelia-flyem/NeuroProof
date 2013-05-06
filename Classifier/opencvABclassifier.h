
#ifndef _opencv_ab_classifier
#define _opencv_ab_classifier

#include "opencv/ml.h"

#include "edgeclassifier.h"

using namespace std;


class OpencvABclassifier: public EdgeClassifier{

    CvBoost* _ab;	
    int _nfeatures;
    int _nclass;	
	

public:
     OpencvABclassifier():_ab(NULL){};	
     OpencvABclassifier(const char* rf_filename);
     ~OpencvABclassifier(){
	 if (_ab) delete _ab;
     }	
     void  load_classifier(const char* rf_filename);
     double predict(std::vector<double>& features);
     void learn(std::vector< std::vector<double> >& pfeatures, std::vector<int>& plabels);
     void save_classifier(const char* rf_filename);
     bool is_trained(){
	if (_ab && _ab->get_weak_predictors()->total >0)
	   return true;
	else 
	   return false;
		
     };

};

#endif
