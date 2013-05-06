#include "DataStructures/Stack.h"
#include "Priority/GPR.h"
#include "Priority/LocalEdgePriority.h"
#include "Utilities/ScopeTime.h"
#include "ImportsExports/ImportExportRagPriority.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <memory>
#include <json/json.h>
#include <json/value.h>

#include "Utilities/h5read.h"
#include "Utilities/h5write.h"

#include <ctime>
#include <cmath>
#include <cstring>

using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using namespace NeuroProof;
using std::vector;

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
    int          i, j, k;


    cout<< "Reading data ..." <<endl;



    if (argc<15){
	printf("format: NeuroProof_stack -watershed watershed_h5_file  dataset \
					 -prediction prediction_h5_file dataset \
					 -classifier classifier_file \
					 -output output_h5_file dataset \
					 -groundtruth groundtruth_h5_file dataset \
					 -threshold agglomeration_threshold \
					 -algorithm algorithm_type\n");
	return 0;
    }	

    int argc_itr=1;	
    string watershed_filename="";
    string watershed_dataset_name="";		
    string prediction_filename="";
    string prediction_dataset_name="";		
    string output_filename;
    string output_dataset_name;		
    string groundtruth_filename="";
    string groundtruth_dataset_name="";		
    string classifier_filename;

    string output_filename_nomito;

    	
    double threshold = 0.2;	
    int agglo_type = 1;		
    bool merge_mito = true;
    bool merge_mito_by_chull = false;
    bool read_off_rwts = false;
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
	if (!(strcmp(argv[argc_itr],"-output"))){
	    output_filename = argv[++argc_itr];
	    output_dataset_name = argv[++argc_itr];
	}
	if (!(strcmp(argv[argc_itr],"-groundtruth"))){
	    groundtruth_filename = argv[++argc_itr];
	    groundtruth_dataset_name = argv[++argc_itr];
	}

	if (!(strcmp(argv[argc_itr],"-threshold"))){
	    threshold = atof(argv[++argc_itr]);
	}
	if (!(strcmp(argv[argc_itr],"-algorithm"))){
	    agglo_type = atoi(argv[++argc_itr]);
	}
	if (!(strcmp(argv[argc_itr],"-nomito"))){
	    merge_mito = false;
	}
	if (!(strcmp(argv[argc_itr],"-mito_chull"))){
	    merge_mito_by_chull = true;
	}
	if (!(strcmp(argv[argc_itr],"-read_off"))){
	    if (agglo_type==2)
		read_off_rwts = true;
	}
        ++argc_itr;
    } 	
    	
    output_filename_nomito = output_filename;
    size_t ofn = output_filename_nomito.find_last_of(".");
    output_filename_nomito.erase(ofn-1,1);			

    time_t start, end;
    time(&start);	

    H5Read watershed(watershed_filename.c_str(),watershed_dataset_name.c_str());	
    Label* watershed_data=NULL;	
    watershed.readData(&watershed_data);	
    int depth =	 watershed.dim()[0];
    int height = watershed.dim()[1];
    int width =	 watershed.dim()[2];

	
    int pad_len=1;
    Label *zp_watershed_data=NULL;
    padZero(watershed_data, watershed.dim(),pad_len,&zp_watershed_data);	


    StackPredict* stackp = new StackPredict(zp_watershed_data, depth+2*pad_len, height+2*pad_len, width+2*pad_len, pad_len);
    stackp->set_feature_mgr(new FeatureMgr());
	

    H5Read prediction(prediction_filename.c_str(),prediction_dataset_name.c_str());	
    float* prediction_data=NULL;
    prediction.readData(&prediction_data);	
 
    double* prediction_single_ch = new double[depth*height*width];
    double* prediction_ch0 = new double[depth*height*width];
    	

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
		    prediction_single_ch[count] = prediction_data[position];
		    count++;					
		}		
	    }	
	}
	
	double* zp_prediction_single_ch = NULL;
	padZero(prediction_single_ch,watershed.dim(),pad_len,&zp_prediction_single_ch);
	if (ch == 0)
	    memcpy(prediction_ch0, prediction_single_ch, depth*height*width*sizeof(double));	

        stackp->add_empty_channel();
	stackp->add_prediction_channel(zp_prediction_single_ch);
    }

    stackp->set_basic_features();

    EdgeClassifier* eclfr;
    if (endswith(classifier_filename, ".h5"))
    	eclfr = new VigraRFclassifier(classifier_filename.c_str());	
    else if (endswith(classifier_filename, ".xml")) 	
	eclfr = new OpencvRFclassifier(classifier_filename.c_str());	

    stackp->get_feature_mgr()->set_classifier(eclfr);   	 

    Label* groundtruth_data=NULL;
    if (groundtruth_filename.size()>0){  	
	H5Read groundtruth(groundtruth_filename.c_str(),groundtruth_dataset_name.c_str());	
        groundtruth.readData(&groundtruth_data);	
        stackp->set_groundtruth(groundtruth_data);
    }	
	

    cout<<"Building RAG ..."; 	
    stackp->build_rag();
    cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	
    cout<<"Inclusion removal ..."; 
    stackp->remove_inclusions();
    cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	

    stackp->compute_vi();  	
    stackp->compute_groundtruth_assignment();

    if (agglo_type==0){	
	cout<<"Agglomerating (flat) upto threshold "<< threshold<< " ...\n"; 
    	stackp->agglomerate_rag_flat(threshold);	
    }
    else if (agglo_type==1){
	cout<<"Agglomerating (agglo) upto threshold "<< threshold<< " ...\n"; 
     	stackp->agglomerate_rag(threshold, false);	
    }
    else if (agglo_type == 2){		
	cout<<"Agglomerating (mrf) upto threshold "<< threshold<< " ...\n"; 
    	stackp->agglomerate_rag_mrf(threshold, read_off_rwts, output_filename, classifier_filename);	
    }		
    else if (agglo_type == 3){		
	cout<<"Agglomerating (queue) upto threshold "<< threshold<< " ...\n"; 
    	stackp->agglomerate_rag_queue(threshold, false);	
    }		
    else if (agglo_type == 4){		
	cout<<"Agglomerating (flat) upto threshold "<< threshold<< " ...\n"; 
    	stackp->agglomerate_rag_flat(threshold, false, output_filename, classifier_filename);	
    }		
    cout << "Done with "<< stackp->get_num_bodies()<< " regions\n";

    cout<<"Inclusion removal ..."; 
    stackp->remove_inclusions();
    cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	



    stackp->compute_vi();  	
 	
    Label * temp_label_volume1D = stackp->get_label_volume();       	    
     if (!merge_mito)
       stackp->absorb_small_regions2(prediction_ch0, temp_label_volume1D);

    hsize_t dims_out[3];
    dims_out[0]=depth; dims_out[1]= height; dims_out[2]= width;   
    H5Write(output_filename_nomito.c_str(),output_dataset_name.c_str(),3,dims_out, temp_label_volume1D);
    printf("Output-nomito written to %s, dataset %s\n",output_filename_nomito.c_str(),output_dataset_name.c_str());	
    delete[] temp_label_volume1D;	


    if (merge_mito){
	cout<<"Merge Mitochondria (border-len) ..."; 
	stackp->merge_mitochondria_a();
    	cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	

        cout<<"Inclusion removal ..."; 
        stackp->remove_inclusions();
        cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	

        stackp->compute_vi();  	
 	
        temp_label_volume1D = stackp->get_label_volume();       	    
         stackp->absorb_small_regions2(prediction_ch0, temp_label_volume1D);

        dims_out[0]=depth; dims_out[1]= height; dims_out[2]= width;   
        H5Write(output_filename.c_str(),output_dataset_name.c_str(),3,dims_out, temp_label_volume1D);
        printf("Output written to %s, dataset %s\n",output_filename.c_str(),output_dataset_name.c_str());	
        delete[] temp_label_volume1D;	
    } 	
    //stackp->determine_edge_locations();
    //stackp->write_graph(output_filename);

    time(&end);	
    printf("Time elapsed: %.2f\n", (difftime(end,start))*1.0/60);



    if (watershed_data)  	
	delete[] watershed_data;
    delete[] zp_watershed_data;
	
    if (prediction_data)  	
	delete[] prediction_data;
    delete[] prediction_single_ch;

    if (groundtruth_data)  	
	delete[] groundtruth_data;

     	

    return 0;
}
