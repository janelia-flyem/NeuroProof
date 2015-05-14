#include "opencvABclassifier.h"
#include "assert.h"
#include <time.h>
#include <stdio.h>

OpencvABclassifier::OpencvABclassifier(const char* rf_filename){

    _ab=NULL;	
    load_classifier(rf_filename);
}

void  OpencvABclassifier::load_classifier(const char* rf_filename){
    if(!_ab)
        _ab = new CvBoost;
    _ab->load(rf_filename);	
    printf("RF loaded with %d trees\n",_ab->get_weak_predictors()->total);
     
}

double OpencvABclassifier::predict(std::vector<double>& pfeatures){
    if(!_ab){
        return(EdgeClassifier::predict(pfeatures));
    }
	//assert(_nfeatures == features.size());

    int nfeatures = pfeatures.size();
    CvMat* features = cvCreateMat(1, nfeatures, CV_32F);	
    float* datap = features->data.fl; 	
    for(int i=0;i< nfeatures;i++)
	datap[i] = pfeatures[i];


    CvMat* weak_responses = cvCreateMat( 1, _ab->get_weak_predictors()->total, CV_32F ); 

    double r = _ab->predict( features, 0 , weak_responses );

    double sum_resp=0;
    double sum_coeff=0;	
    for (int j = 0; j< weak_responses->cols ; j++){
	sum_coeff += fabs(weak_responses->data.fl[j]);
	sum_resp += ( (weak_responses->data.fl[j]> 0) ? fabs(weak_responses->data.fl[j]) : 0);	
    }
    double prob = sum_resp / sum_coeff;


    cvReleaseMat( &features );
    cvReleaseMat( &weak_responses );

    return  prob;
			    	
}	


void OpencvABclassifier::learn(std::vector< std::vector<double> >& pfeatures, std::vector<int>& plabels){

     if (_ab){
	delete _ab;	
     }
     _ab = new CvBoost;
 	

     int rows = pfeatures.size();
     int cols = pfeatures[0].size();	 	
     
     printf("Number of samples and dimensions: %d, %d\n",rows, cols);
     if ((rows<1)||(cols<1)){
	return;
     }

     clock_t start = clock();

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


     int tre_count = 255;	

     // 1. create type mask
     CvMat* var_type = cvCreateMat( features->cols + 1, 1, CV_8U );
     cvSet( var_type, cvScalarAll(CV_VAR_NUMERICAL) );
     cvSetReal1D( var_type, features->cols, CV_VAR_CATEGORICAL );

     // define the parameters for training adaboost (trees)
     float priors[] = {1,1};

     // set the boost parameters

     CvBoostParams params = CvBoostParams(CvBoost::DISCRETE,  // boosting type
                                         tre_count,	 // number of weak classifiers
                                         0.95,  	 // trim rate

                                             // trim rate is a threshold (0->1)
                                             // used to eliminate samples with
                                             // boosting weight < 1.0 - (trim rate)
                                             // from the next round of boosting
                                             // Used for computational saving only.

                                        20,   // max depth of trees
                                        false,  // compute surrogate split, no missing data
                                        priors );

        // as CvBoostParams inherits from CvDTreeParams we can also set generic
        // parameters of decision trees too (otherwise they use the defaults)

     params.max_categories = 15; 	// max number of categories (use sub-optimal algorithm for larger numbers)
     params.min_sample_count = 5; 	// min sample count:  minimum samples required at a leaf node for it to be split
     params.cv_folds = 1;		// cross validation folds
     params.use_1se_rule = false; 	// use 1SE rule => smaller tree: more noise tolerant less accurate
     params.truncate_pruned_tree = false; // throw away the pruned tree branches
     params.regression_accuracy = 0.0; 	// regression accuracy: N/A here


        // train boosted tree classifier (using training data)

     fflush(NULL);


     _ab->train( features, CV_ROW_SAMPLE, labels, 0, 0, var_type,
                          0, params, false);

     CvMat* weak_responses = cvCreateMat( 1, _ab->get_weak_predictors()->total, CV_32F ); 

     double correct = 0;
     for(int  i = 0; i < features->rows ; i++ ){
         double r;
         CvMat sample;
         cvGetRow( features, &sample, i );
	

    	 r = _ab->predict( &sample, 0 , weak_responses );
    	 double sum_resp=0;
    	 double sum_coeff=0;	
	 for (int j = 0; j< weak_responses->cols ; j++){
	     sum_coeff += fabs(weak_responses->data.fl[j]);
	     sum_resp += ( (weak_responses->data.fl[j]> 0) ? fabs(weak_responses->data.fl[j]) : 0);	
    	 }
    	 double prob = sum_resp / sum_coeff;

	 r = (prob>0.5? 1 :-1);
         r = fabs((double)r - labels->data.fl[i]) <= FLT_EPSILON ? 1 : 0;


	 correct += r;
      }
      	

     printf("Time required to learn RF: %.2f\n", ((double)clock() - start) / CLOCKS_PER_SEC);
     printf("with training set accuracy :%.3f\n", correct/features->rows*100.);

     cvReleaseMat( &features );
     cvReleaseMat( &weak_responses );
     cvReleaseMat( &labels );	
     cvReleaseMat( &var_type );	
}

void OpencvABclassifier::save_classifier(const char* rf_filename){
    _ab->save(rf_filename);	
}

