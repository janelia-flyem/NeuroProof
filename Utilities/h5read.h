#ifndef _H5_READ
#define _H5_READ

#include "hdf5.h"

class H5Read{

    hid_t       file, dataset;         /* handles */
    hid_t       datatype, dataspace, nativetype;
    H5T_class_t t_class;                 /* data type class */
    size_t      size;                  /*
				        * size of the data element
				        * stored in file
				        */
    hsize_t*     dims_out;           /* dataset dimensions */

    size_t  rank;  	
	
    int          i, j, k;

    bool verbose;	

public:
    ~H5Read(){	
     	if (dims_out)
		delete [] dims_out;	 
	if (datatype)
	    H5Tclose(datatype);
	if (nativetype)
	    H5Tclose(nativetype);
	if (dataset)
	    H5Dclose(dataset);
	if (dataspace)
	    H5Sclose(dataspace);
	if (file)
	    H5Fclose(file);
    }	
    H5Read(const char* h5filename,const char *datasetname, bool pverbose=false){
	Initiate((char*)h5filename,(char*) datasetname, pverbose);
    }
    H5Read(char* h5filename, char *datasetname, bool pverbose=false){
	Initiate(h5filename,datasetname, pverbose);
    }
	
    void Initiate(char* h5filename, char *datasetname, bool pverbose=false){
	verbose = pverbose;	

	int status_n;
	dims_out=NULL;
	file=0;
  	dataset=0;
	dataspace=0;
	datatype=0;
	nativetype=0;

    	file = H5Fopen(h5filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    	if (file<0){
             printf("h5 file could not be opened\n");
             return;
        }
	dataset = H5Dopen2(file, datasetname, H5P_DEFAULT);
	if (dataset<0){
             printf("dataset not found\n");
             return;
	}


	datatype  = H5Dget_type(dataset);     /* datatype handle */
	nativetype= H5Tget_native_type(datatype,H5T_DIR_ASCEND);
    	t_class     = H5Tget_class(datatype);
	
	if (verbose){
            if (t_class == H5T_INTEGER)
	        printf("Data set has INTEGER type\n");
	    else if (t_class == H5T_FLOAT)
	  	printf("Data set has FLOAT type \n");
	}

	size  = H5Tget_size(datatype);
    	//printf(" Data size is %d \n", (int)size);


	dataspace = H5Dget_space(dataset);    /* dataspace handle */
    	rank      = H5Sget_simple_extent_ndims(dataspace); 
    	dims_out = new hsize_t[rank];
	if (verbose) printf("number of dimensions : %d\n",rank);
	status_n  = H5Sget_simple_extent_dims(dataspace, dims_out, NULL);
	if (verbose){
	    printf("Dimensions: ");

	    int large_data=0; 	
	    for (j=0;j<rank;j++){
                printf("%lu   ",(unsigned int)(dims_out[j]) );
            }
            printf("\n");
	}

    }

    template <class T> 	
    int readData(T **pdata){

	unsigned long int total_size=1;
	for(j=0;j<rank;j++)
	    total_size *= dims_out[j];	
	
	if (verbose) printf("total data size = %d\n",total_size);

	(*pdata) = new T[total_size];
	T* data = *pdata;
	

	herr_t status = H5Dread(dataset, nativetype, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
	if (status<0){
            printf(" data read not successful\n");	
	    return 0; 	
        }
    }
    const size_t* dim(){
	return (size_t*)dims_out;
    }

    size_t total_dim(){
  	return rank;
    }	
};


#endif
