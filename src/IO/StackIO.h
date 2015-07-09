/*!
 * Defines functions for loading in label volumes and stack data.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKIO_H
#define STACKIO_H

#include <json/json.h>
#include <json/value.h>
#include <Stack/Stack.h>
#include <Stack/VolumeLabelData.h>

// used for importing h5 files
#include <vigra/hdf5impex.hxx>
#include <vigra/impex.hxx>

namespace NeuroProof {

/*!
 * Initializes from a stack h5 file
 * \param stack_name name of stack h5 file
*/
Stack import_h5stack(std::string stack_name);

/*!
 * Initializes a substack from DVID
 * \param json_name dvid config file
*/
Stack import_dvidstack(std::string json_name);

/*!
 * Function to create a volume label data object
 * from an h5 file.  Input h5 files are assummed to be Z,Y,X.
 * If this h5 file contains a transform dataset, the labels in the
 * label volume dataset will be mapped to new labels according
 * to the transform.  TODO: allow user-defined axis specification.
 * \param h5_name name of h5 file
 * \param dset name of dataset
 * \param use_tranforms decide whether to use transforms or not
 * \return shared pointer to volum label data
*/
VolumeLabelPtr import_h5labels(const char * h5_name, const char* dset,
        bool use_transforms = true);


/*!
 * function to create a volume data object from 
 * an h5 file. For now, input h5 files are assumed to be Z x Y x X.
 * TODO: allow user-defined axis specification.
 * \param h5_name name of h5 file
 * \param dset name of dataset
 * \return shared pointer to volume data
*/
template <typename T>
boost::shared_ptr<VolumeData<T> > import_3Dh5vol(
        const char * h5_name, const char * dset);

/*!
 * Function to create an array of volume data from an h5
 * assumed to have format X x Y x Z x num channels.  Each channel,
 * will define a new volume data in the array. TODO: allow user-defined
 * axis specification.
 * \param h5_name name of h5 file
 * \param dset name of dset
 * \return vector of shared pointers to volume data
*/
template <typename T>
std::vector<boost::shared_ptr<VolumeData<T> > >
    import_3Dh5vol_array(const char * h5_name, const char * dset);


/*!
 * Function to create an array of volume data from an h5
 * assumed to have format X x Y x Z x num channels.  Each channel,
 * will define a new volume data in the array. The dim1size field indicates
 * that a subset of the data will be used.  TODO: allow user-defined
 * axis specification.
 * \param h5_name name of h5 file
 * \param dset name of dset
 * \param dim1size size of the first dimension of a companion volume
 * \return vector of shared pointers to volume data
*/
template <typename T>
std::vector<boost::shared_ptr<VolumeData<T> > >
    import_3Dh5vol_array(const char * h5_name, const char * dset, unsigned int dim1size);


/*!
 * Function to create a 3D image volume from a list of
 * 2D image files.  While many 2D image formats are supported
 * (see vigra documentation), this function will only work with
 * 8-bit, grayscale values for now.
 * \param file_names array of images in correct order
 * \return shared pointer to volume data
*/
boost::shared_ptr<VolumeData<unsigned char> > import_8bit_images(
    std::vector<std::string>& file_names);

/*!
 * Write volume data to disk assuming Z x Y x X in h5 output.
 * \param h5_name name of h5 file
 * \param h5_path path to h5 dataset
*/
template <typename T>
void export_3Dh5vol(boost::shared_ptr<VolumeData<T> > volume, 
        const char* h5_name, const char * h5_path); 

/*!
 * Write label volume data to disk assuming Z x Y x X in h5 output.
 * \param h5_name name of h5 file
 * \param h5_path path to h5 dataset
*/
void export_3Dh5vol(VolumeLabelPtr volume, 
        const char* h5_name, const char * h5_path); 



/*!
 * Write the stack to disk.  The graph is written to the json
 * file format.  The label volume is rebased and written
 * to an h5 file.  There is an option to determine the best location
 * for observing the edge using the information in the probability volumes.
 * \param h5_name name of h5 file
 * \param graph_name name of graph json file
 * \param optimal_prob_edge_loc determine strategy to select edge location
 * \param disable_prob_comp determines whether saved prob values are used
*/
void export_stack(Stack* stack, const char* h5_name, const char* graph_name,
        bool optimal_prob_edge_loc, bool disable_prob_comp = false);

/*!
 * Creates a set of labels from a json file and zeros out these labels
 * in the label volume.
 * \param exclusions_json name of json file with exclusion labels
*/
void import_stack_exclusions(Stack* stack, std::string exclusions_json);

//// TEMPLATE IMPLEMENTATIONS

template <typename T>
boost::shared_ptr<VolumeData<T> > import_3Dh5vol(
        const char * h5_name, const char * dset)

{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 3, "Dataset must be 3-dimensional.");

    boost::shared_ptr<VolumeData<T> > volumedata = VolumeData<T>::create_volume();

    vigra::TinyVector<long long unsigned int,3> shape(info.shape().begin());
    volumedata->reshape(shape);
    // read h5 file into volumedata with correctly set shape
    vigra::readHDF5(info, *volumedata);

    return volumedata; 
}

template <typename T>
std::vector<boost::shared_ptr<VolumeData<T> > > import_3Dh5vol_array(
        const char * h5_name, const char * dset)
{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 4, "Dataset must be 4-dimensional.");

    vigra::TinyVector<long long unsigned int,4> shape(info.shape().begin());
    vigra::MultiArray<4, T> volumedata_temp(shape);
    vigra::readHDF5(info, volumedata_temp);
    
    // since the X,Y,Z,ch is read in as ch,Z,Y,X transpose
    volumedata_temp = volumedata_temp.transpose();

    std::vector<VolumeProbPtr> vol_array;
    vigra::TinyVector<long long unsigned int,3> shape2;

    // tranpose the shape dimensions as well
    shape2[0] = shape[3];
    shape2[1] = shape[2];
    shape2[2] = shape[1];

    // for each channel, create volume data and push in array
    for (int i = 0; i < shape[0]; ++i) {
        boost::shared_ptr<VolumeData<T> > volumedata = VolumeData<T>::create_volume();
        vigra::TinyVector<vigra::MultiArrayIndex, 1> channel(i);
        (*volumedata) = volumedata_temp.bindOuter(channel); 
        
        vol_array.push_back(boost::shared_ptr<VolumeData<T> >(volumedata));
    }

    return vol_array; 
}

template <typename T>
std::vector<boost::shared_ptr<VolumeData<T> > > 
    import_3Dh5vol_array(const char * h5_name, const char * dset,
            unsigned int dim1size)
{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 4, "Dataset must be 4-dimensional.");

    vigra::TinyVector<long long unsigned int,4> shape(info.shape().begin());
    vigra::MultiArray<4, T> volumedata_temp(shape);
    vigra::readHDF5(info, volumedata_temp);
    
    // since the X,Y,Z,ch is read in as ch,Z,Y,X transpose
    volumedata_temp = volumedata_temp.transpose();

    std::vector<VolumeProbPtr> vol_array;
    vigra::TinyVector<long long unsigned int,3> shape2;

    // tranpose the shape dimensions as well
    shape2[0] = shape[3];
    shape2[1] = shape[2];
    shape2[2] = shape[1];

    // prediction must be the same size or larger than the label volume
    if (dim1size > shape2[0]) {
        throw ErrMsg("Label volume has a larger dimension than the prediction volume provided");
    }
    
    // extract border from shape and size of label volume
    unsigned int border = (shape2[0] - dim1size) / 2;

    // if a border needs to be applied the volume should be equal size in all dimensions
    // TODO: specify borders for each dimension
    if (border > 0) {
        if ((shape2[0] != shape2[1]) || (shape2[0] != shape2[2])) {
            throw ErrMsg("Dimensions of prediction should be equal in X, Y, Z");
        }
    }



    // for each channel, create volume data and push in array
    for (int i = 0; i < shape[0]; ++i) {
        boost::shared_ptr<VolumeData<T> > volumedata = VolumeData<T>::create_volume();
        vigra::TinyVector<vigra::MultiArrayIndex, 1> channel(i);
        (*volumedata) = (volumedata_temp.bindOuter(channel)).subarray(
                vigra::Shape3(border, border, border), vigra::Shape3(shape2[0]-border,
                    shape2[1]-border, shape2[2]-border)); 
        
        vol_array.push_back(boost::shared_ptr<VolumeData<T> >(volumedata));
    }

    return vol_array; 
}

template <typename T>
void export_3Dh5vol(boost::shared_ptr<VolumeData<T> > volume, 
        const char* h5_name, const char * h5_path)
{
    // x,y,z data will be written as z,y,x in the h5 file by default
    vigra::writeHDF5(h5_name, h5_path, *volume);
}



}


#endif
