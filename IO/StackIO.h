/*!
 * Defines functions for loading in label volumes and stack data.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKIO_H
#define STACKIO_H

#include <json/json.h>
#include <json/value.h>

namespace NeuroProof {

/*!
 * Initializes from a stack h5 file
 * \param stack_name name of stack h5 file
*/
Stack import_stack(std::string stack_name);



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



}


#endif
