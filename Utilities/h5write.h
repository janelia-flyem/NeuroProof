#ifndef _H5_WRITE
#define _H5_WRITE

#include "hdf5.h"

class H5Write{


public:

    H5Write(const char* filename, const char* datasetname, int rank, const hsize_t* dimsf, 
            unsigned int* pdata, const char* datasetname2, int rank2, const hsize_t* dimsf2,
            unsigned long long* pdata2, bool modify) {
	
	hid_t       file, dataset, dataset2;         /* file and dataset handles */
	hid_t       datatype, dataspace, dataspace2;   /* handles */
	herr_t      status;



	/*
	 * Create a new file using H5F_ACC_TRUNC access,
	 * default file creation properties, and default file
	 * access properties.
	 */
	
        if (!modify) {
            file = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        } else {
            file = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
        }
	
        if (file<0){
	    printf("Error: H5 file could not be opened for writing\n");	
	    exit(-1);	
	}
       
        // remove datasets if they previously exist (could spit out
        // warnings if the dataset does not exist 
        if (modify) {
            status = H5Ldelete(file, datasetname, H5P_DEFAULT);
            status = H5Ldelete(file, datasetname2, H5P_DEFAULT);
        }

	/*
	 * Describe the size of the array and create the data space for fixed
	 * size dataset.
	 */
	dataspace = H5Screate_simple(rank, dimsf, NULL);
	dataspace2 = H5Screate_simple(rank2, dimsf2, NULL);
	if (dataspace<0){
	    printf("Error: dataspace creation\n");	
	    exit(-1);
        }
	if (dataspace2<0){
	    printf("Error: dataspace creation\n");	
	    exit(-1);	
	}


	/*
	 * Create a new dataset within the file using defined dataspace and
	 * datatype and default dataset creation properties.
	 */
	dataset = H5Dcreate2(file, datasetname, H5T_NATIVE_UINT, dataspace,
			    H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if (dataset<0){
	    printf("Error: dataset could not be written\n");	
	    exit(-1);	
	}


	/*
	 * Create a new dataset within the file using defined dataspace and
	 * datatype and default dataset creation properties.
	 */
	dataset2 = H5Dcreate2(file, datasetname2, H5T_NATIVE_UINT64, dataspace2,
			    H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if (dataset<0){
	    printf("Error: dataset could not be written\n");	
	    exit(-1);	
	}


        /*
	 * Write the data to the dataset using default transfer properties.
	 */
	status = H5Dwrite(dataset, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT,  pdata);
	status = H5Dwrite(dataset2, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT,  pdata2);

	/*
	 * Close/release resources.
	 */
	H5Sclose(dataspace);
	H5Sclose(dataspace2);
	H5Dclose(dataset);
	H5Dclose(dataset2);
	H5Fclose(file);
    }	
};

#endif
