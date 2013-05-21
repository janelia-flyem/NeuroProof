#include "DataStructures/Stack.h"
#include "Priority/GPR.h"
#include "Priority/LocalEdgePriority.h"
#include "Utilities/ScopeTime.h"
#include "Rag/RagIO.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <memory>
#include <json/json.h>
#include <json/value.h>

#include "Utilities/h5read.h"
#include "Utilities/h5write.h"

#include <time.h>

using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using namespace NeuroProof;
using namespace std;



template <class T>
void padZero(T* data, const size_t* dims, int pad_length, T** ppadded_data){
     
     // implemented only for 3D arrays

     unsigned long int newsize = (dims[0]+2*pad_length)*(dims[1]+2*pad_length)*(dims[2]+2*pad_length);	
     *ppadded_data = new T[newsize];  	
     T* padded_data = *ppadded_data;
	
     memset((void*) padded_data, 0, sizeof(T)*newsize);

     unsigned int width, plane_size, width0, plane_size0, i0,j0,k0, i,j,k;
     
     for (i=pad_length, i0=0; i<= dims[0] ; i++, i0++)
	 for (j=pad_length, j0=0; j<= dims[1]; j++, j0++)	
	     for(k=pad_length, k0=0; k<= dims[2]; k++, k0++){
		 plane_size = (dims[1]+2*pad_length)*(dims[2]+2*pad_length);	
		 width = dims[2]+2*pad_length;	

		 plane_size0 = dims[1]*dims[2];	
		 width0 = dims[2];	

		 padded_data[i*plane_size+ j*width + k] = data[i0*plane_size0 + j0*width0 + k0];	
	     }
			

}
bool endswith(string filename, string extn){
    unsigned found = filename.find_last_of(".");
    string fextn = filename.substr(found);	
    if (fextn.compare(extn) == 0 )
	return true;
    else return false;	  
}

int main(int argc, char** argv) 
{

    cout<< "Reading data ..." <<endl;
    size_t          i, j, k;



    if (argc<16){
	printf("format: NeuroProof_stack_learn -watershed watershed_h5_file  dataset \
					 -prediction prediction_h5_file dataset \
					 -groundtruth groundtruth_h5_file dataset \
					 -iteration num_iteration \
					 -strategy learning_strategy \
					 -classifier classifier_file \n");
	return 0;
    }	

    int argc_itr=1;	
    string watershed_filename="";
    string watershed_dataset_name="";		
    string prediction_filename="";
    string prediction_dataset_name="";		
    string groundtruth_filename="";
    string groundtruth_dataset_name="";		
    string classifier_filename;
    	
    int maxIter = 1; 	
    int strategy = 1; 	
    while(argc_itr<argc){
	if (!(strcmp(argv[argc_itr],"-watershed"))){
	    watershed_filename = argv[++argc_itr];
	    watershed_dataset_name = argv[++argc_itr];
	}

	if (!(strcmp(argv[argc_itr],"-classifier"))){
	    classifier_filename = argv[++argc_itr];
	}
	if (!(strcmp(argv[argc_itr],"-prediction"))){
	    prediction_filename = argv[++argc_itr];
	    prediction_dataset_name = argv[++argc_itr];
	}
	if (!(strcmp(argv[argc_itr],"-groundtruth"))){
	    groundtruth_filename = argv[++argc_itr];
	    groundtruth_dataset_name = argv[++argc_itr];
	}

	if (!(strcmp(argv[argc_itr],"-iteration"))){
	    maxIter = atoi(argv[++argc_itr]);
	}
	if (!(strcmp(argv[argc_itr],"-strategy"))){
	    strategy = atoi(argv[++argc_itr]);
	}
        ++argc_itr;
    } 	


    time_t start, end;
    time(&start);	

    //clock_t start = clock();

    H5Read watershed(watershed_filename.c_str(), watershed_dataset_name.c_str());	
    Label* watershed_data=NULL;	
    watershed.readData(&watershed_data);	
    int depth =	 watershed.dim()[0];
    int height = watershed.dim()[1];
    int width =	 watershed.dim()[2];


	

    H5Read groundtruth(groundtruth_filename.c_str(),groundtruth_dataset_name.c_str());	
    Label* groundtruth_data=NULL;
    groundtruth.readData(&groundtruth_data);	




    int pad_len=1;
    Label *zp_watershed_data=NULL;
    padZero(watershed_data, watershed.dim(),pad_len,&zp_watershed_data);	

    Label *zp_groundtruth_data=NULL;
    padZero(groundtruth_data, groundtruth.dim(),pad_len,&zp_groundtruth_data);	
	

    H5Read prediction(prediction_filename.c_str(), prediction_dataset_name.c_str());	
    float* prediction_data=NULL;
    prediction.readData(&prediction_data);	
    double* prediction_ch1 = new double[depth*height*width];





    double threshold=0.2;

    EdgeClassifier* eclfr;
    if (endswith(classifier_filename, ".h5"))
    	eclfr = new VigraRFclassifier();	
    else if (endswith(classifier_filename, ".xml")) 	
	eclfr = new OpencvRFclassifier();	

    UniqueRowFeature_Label	all_features;
    vector<int> all_labels;	

    for(int itr=0 ; itr < maxIter ; itr++){

	printf("\n ** Learning iteration %d  **\n\n",itr+1);
	
	StackLearn* stackp = new StackLearn(zp_watershed_data, depth+2*pad_len, height+2*pad_len, width+2*pad_len, pad_len);
	stackp->set_feature_mgr(new FeatureMgr());


	size_t cube_size, plane_size, element_size=prediction.dim()[prediction.total_dim()-1];
	size_t position, count;		    	
	for (int ch=0; ch < element_size; ch++){
	    count = 0;
	    for(i=0; i<depth; i++){
		cube_size = height*width*element_size;	
		for(j=0; j<height; j++){
		    plane_size = width*element_size;
		    for(k=0; k<width; k++){
			position = i*cube_size + j*plane_size + k*element_size + ch;
			prediction_ch1[count] = prediction_data[position];				
			count++;
		    }		
		}	
	    }

	    double* zp_prediction_ch1 = NULL;
	    padZero(prediction_ch1,watershed.dim(),pad_len,&zp_prediction_ch1);
            stackp->add_empty_channel();
	    stackp->add_prediction_channel(zp_prediction_ch1);
	}

	stackp->set_basic_features();  	


	stackp->set_groundtruth(groundtruth_data);	



	stackp-> get_feature_mgr()->set_classifier(eclfr);	 


	cout<<"Learn edge classifier ...\n"; 
	if (itr<1){
	    stackp->learn_edge_classifier_flat(threshold,all_features,all_labels); // # iteration, threshold, clfr_filename
	}
	else{
	    if (strategy == 1){ //accumulate only misclassified 
		printf("cumulative learning, only misclassified\n");
	   	stackp->learn_edge_classifier_queue(threshold,all_features,all_labels, false); // # iteration, threshold, clfr_filename	
	    }
	    else if (strategy == 2){ //accumulate all 
		printf("cumulative learning, all\n");
	   	stackp->learn_edge_classifier_queue(threshold,all_features,all_labels, true); // # iteration, threshold, clfr_filename	
	    }	
	    else if (strategy == 3){ // lash	
		printf("learning by LASH\n");
	   	stackp->learn_edge_classifier_lash(threshold,all_features,all_labels); // # iteration, threshold, clfr_filename	
	    }
	}

	cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	

	delete stackp;

    }
// end for

    eclfr->save_classifier(classifier_filename.c_str());  	
    printf("Classifier saved to %s\n",classifier_filename.c_str());


 	
     
    time(&end);	
    printf("Time elapsed: %.2f\n", (difftime(end,start))*1.0/60);

 
//    printf("Time elapsed: %.2f\n", ((double)clock() - start) / CLOCKS_PER_SEC);


//    if (watershed_data)  	
//	delete[] watershed_data;
//    delete[] zp_watershed_data;

//    if (prediction_data)  	
//	delete[] prediction_data;
  //  delete[] prediction_ch1;	

//    if (groundtruth_data)  	
//	delete[] groundtruth_data;
  //  delete[] zp_groundtruth_data;	

     	

    return 0;
}
