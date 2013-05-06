#ifndef _H5_WRITE
#define _H5_WRITE

#include "hdf5.h"

class H5Write{


public:

    H5Write(const char* filename, const char* datasetname, int rank, const hsize_t* dimsf, unsigned int* pdata){
	
	hid_t       file, dataset;         /* file and dataset handles */
	hid_t       datatype, dataspace;   /* handles */
	//hsize_t     dimsf[2];              /* dataset dimensions */
	herr_t      status;
	//int         data[NX][NY];          /* data to write */
	//int         i, j;


	//printf("%s\n", filename);
	//printf("dataset: %s\n", datasetname);


	/*
	 * Create a new file using H5F_ACC_TRUNC access,
	 * default file creation properties, and default file
	 * access properties.
	 */
	file = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if (file<0){
	    printf("H5 file could not be opened for writing\n");	
	    return;	
	}

	/*
	 * Describe the size of the array and create the data space for fixed
	 * size dataset.
	 */
	dataspace = H5Screate_simple(rank, dimsf, NULL);
	if (dataspace<0){
	    printf("dataspace\n");	
	    return;	
	}

	/*
	 * Define datatype for the data in the file.
	 * We will store little endian INT numbers.
	 */
	datatype = H5Tcopy(H5T_NATIVE_UINT);
	if (datatype<0){
	    printf("datatype\n");	
	    return;	
	}
	status = H5Tset_order(datatype, H5T_ORDER_LE);
	if (status<0){
	    printf("set_order\n");	
	    return;	
	}

	/*
	 * Create a new dataset within the file using defined dataspace and
	 * datatype and default dataset creation properties.
	 */
	dataset = H5Dcreate2(file, datasetname, H5T_NATIVE_UINT, dataspace,
			    H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if (dataset<0){
	    printf("dataset could not be written\n");	
	    return;	
	}

	/*
	 * Write the data to the dataset using default transfer properties.
	 */
	status = H5Dwrite(dataset, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT,  pdata);

	/*
	 * Close/release resources.
	 */
	H5Sclose(dataspace);
	H5Tclose(datatype);
	H5Dclose(dataset);
	H5Fclose(file);
    }	
};

#endif
