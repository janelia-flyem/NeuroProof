#include "opencvSVMclassifier.h"
#include "assert.h"
#include <time.h>
#include <stdio.h>

OpencvSVMclassifier::OpencvSVMclassifier(const char* svm_filename){

    _svm=NULL;	
    load_classifier(svm_filename);
     	
}

void  OpencvSVMclassifier::load_classifier(const char* svm_filename){
    if(!_svm)
        _svm = new CvSVM;
    _svm->load(svm_filename);	
    printf("SVM loaded \n");

    string svm_filename2(svm_filename); 	
    size_t idx= svm_filename2.find_last_of(".");
    string svm_scalename = svm_filename2.substr(0, idx);		
    svm_scalename += "_scale.txt"; 	

    FILE* fps = fopen(svm_scalename.c_str(),"rt"); 
    _lb.clear(); _ub.clear();	
    float minf, maxf; 	
    while (!feof(fps)){	
	fscanf(fps,"%f %f\n",&minf, &maxf);
	_lb.push_back(minf);
	_ub.push_back(maxf);
    }		

    fclose(fps);	
     
}

double OpencvSVMclassifier::predict(std::vector<double>& pfeatures){
    if(!_svm){
        return(EdgeClassifier::predict(pfeatures));
    }
	//assert(_nfeatures == features.size());

    int nfeatures = pfeatures.size();
    CvMat* features = cvCreateMat(1, nfeatures, CV_32F);	
    float* datap = features->data.fl; 	
    for(int i=0;i< nfeatures;i++)
	datap[i] = pfeatures[i];

    for(int i=0; i < features->cols; i++){
        double val = features->data.fl[i];	
	features->data.fl[i] = -1 + 2*(val - _lb[i])/(_ub[i] - _lb[i]);
    }

    float fr = _svm->predict( features, true );// returns the distance from hyperplane

    double prob = 1/(1+exp(-fr)); // convert distance to prob
    		
    cvReleaseMat( &features );

    return  prob;
			    	
}	


void OpencvSVMclassifier::learn(std::vector< std::vector<double> >& pfeatures, std::vector<int>& plabels){

     if (_svm){
	delete _svm;	
     }
     _svm = new CvSVM;
 	

     int rows = pfeatures.size();
     int cols = pfeatures[0].size();	 	
     
     printf("Number of samples and dimensions: %d, %d\n",rows, cols);
     if ((rows<1)||(cols<1)){
	return;
     }

     time_t start, end;
     time(&start);

     CvMat *features = cvCreateMat(rows, cols, CV_32F);	
     CvMat *labels = cvCreateMat(rows, 1 , CV_32F);	
     float* datap = features->data.fl;
     float* labelp = labels->data.fl;	 	


     int numzeros=0; 	
     for(int i=0; i < rows; i++){
	 labelp[i] = plabels[i];
	 numzeros += ( labelp[i] == -1? 1 : 0 );
	 for(int j=0; j < cols ; j++){
	     datap[i*cols+j]  = (float)pfeatures[i][j];	
	 }
     }
     printf("Number of merge: %d\n",numzeros);

     _lb.resize(features->cols);	
     _ub.resize(features->cols);	
     for(int j=0; j < features->cols; j++){
	double minf=1e12, maxf=-1e12;
	for(int i=0; i < features->rows; i++){
	    double val = features->data.fl[i*cols+j];	
	    minf = val < minf ? val: minf;	
	    maxf = val > maxf ? val: maxf;	
	}	
        if ((maxf-minf)<1e-6)
	    maxf += 1e-6;

	_lb[j] = minf;
	_ub[j] = maxf;
	for(int i=0; i < features->rows; i++){
	    double val = features->data.fl[i*cols+j];	
	    features->data.fl[i*cols+j] = -1 + 2*(val-minf)/(maxf-minf);
	}
     }		

     CvSVMParams params;

     CvParamGrid grid_C = CvSVM::get_default_grid(CvSVM::C);

    // train boosted tree classifier (using training data)

     _svm->train_auto(features, labels, 0, 0, params, 5, CvParamGrid(1.0/256, 1024, 2), CvParamGrid(1.0/512,128,2) ); // k-fold cross-validation
     printf( "Done.\n");

     CvSVMParams lparams = _svm->get_params();


     double correct = 0;
     for(int  i = 0; i < features->rows ; i++ ){
         double r;
         CvMat sample;
         cvGetRow( features, &sample, i );
	
         r = _svm->predict( &sample );
         r = fabs((double)r - labels->data.fl[i]) <= FLT_EPSILON ? 1 : 0;

	 correct += r;
      }
      	
     time(&end);	

     printf("Time required to learn SVM: %.2f with training set accuracy :%.3f\n", (difftime(end,start))*1.0/60, correct/features->rows*100.);

     cvReleaseMat( &features );
     cvReleaseMat( &labels );	
}

void OpencvSVMclassifier::save_classifier(const char* svm_filename){
    _svm->save(svm_filename);	

    string svm_filename2(svm_filename); 	
    size_t idx= svm_filename2.find_last_of(".");
    string svm_scalename = svm_filename2.substr(0, idx);		
    svm_scalename += "_scale.txt"; 	

    FILE* fps = fopen(svm_scalename.c_str(),"wt"); 

    for (int j=0; j < _lb.size(); j++){
	fprintf(fps,"%f %f\n",_lb[j],_ub[j]);
    }		

    fclose(fps);	
}


