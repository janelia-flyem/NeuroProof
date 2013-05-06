
#ifndef _opencv_svm_classifier
#define _opencv_svm_classifier

#include "opencv/ml.h"

#include "edgeclassifier.h"

using namespace std;
using namespace cv;


class OpencvSVMclassifier: public EdgeClassifier{

    CvSVM* _svm;	
    int _nfeatures;
    int _nclass;	

    vector<double> _lb;
    vector<double> _ub;		

public:
     OpencvSVMclassifier():_svm(NULL){};	
     OpencvSVMclassifier(const char* svm_filename);
     ~OpencvSVMclassifier(){
	 if (_svm) delete _svm;
     }	
     void  load_classifier(const char* svm_filename);
     double predict(std::vector<double>& features);
     void learn(std::vector< std::vector<double> >& pfeatures, std::vector<int>& plabels);
     void save_classifier(const char* svm_filename);

     bool is_trained(){
	if (_svm)
	   return true;
	else 
	   return false;
		
     };

};

#endif
