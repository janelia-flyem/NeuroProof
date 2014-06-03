/*!
 * Defines a class for loading in a label data volume
 * where several labels can be mapped to the same label given
 * a separate transform table.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef VOLUMELABELDATA_H
#define VOLUMELABELDATA_H

#include "VolumeData.h"
#include <vector>
#include <tr1/unordered_map>
#include "../Utilities/Glb.h"

namespace NeuroProof {

class VolumeLabelData;

// unsigned int is the default label data-type used -- this should
// ideally be the same as the Node_t type
typedef Index_t Label_t;
typedef boost::shared_ptr<VolumeLabelData> VolumeLabelPtr; 

/*!
 * Inherits from the VolumeData class with Label_t template parameter.
 * This class defines interface for creating label volumes and provides
 * a mechanism for maintaining mappings of labels to labels enabling
 * one to merge different labels together with generally O(1) computation.
*/
class VolumeLabelData : public VolumeData<Label_t> {
  public:
    /*!
     * Static function to craete an empty volume label object.
     * \return shared pointer to volume label data
    */
    static VolumeLabelPtr create_volume();
   
    /*!
     * Static function to create a volume label data object
     * from an h5 file.  Input h5 files are assummed to be Z,Y,X.
     * If this h5 file contains a transform dataset, the labels in the
     * label volume dataset will be mapped to new labels according
     * to the transform.  TODO: allow user-defined axis specification.
     * \param h5_name name of h5 file
     * \param dset name of dataset
     * \param use_tranforms decide whether to use transforms or not
     * \return shared pointer to volum label data
    */
    static VolumeLabelPtr create_volume(const char * h5_name, const char* dset,
            bool use_transforms = true);

    /*!
     * Static function to create a volume label data object
     * from the given dimensions. 
     * \param xsize is x dimension
     * \param ysize is y dimension
     * \param zsize is z dimension
     * \return shared pointer to volume label data
    */
    static VolumeLabelPtr create_volume(int xsize, int ysize, int zsize);

    /*!
     * Enable the merging of two labels by assigning an old label to
     * another label.  This assignment is done through a hash
     * datastructure and generally takes amortized O(1) runtime.
     * \param old_label label to be replaced
     * \param new_label new label id to replace old label
    */
    void reassign_label(Label_t old_label, Label_t new_label); 
  
    /*!
     * Checks if the given label is mapped to another value.
     * \param label volume label
     * \return true if it has a mapping
    */
    bool is_mapped(Label_t label)
    {
        return label_mapping.find(label) != label_mapping.end();        
    }
 
    /*!
     * Split a given label into two partitons.  This command will not work
     * if the volume was recently rebased.  The specified labels being split
     * off must exist in the mappings.  The first label id in the split labels
     * defines the new label id created.
     * \param curr_label the label being split
     * \param split_labels vector of labels being split off of curr_label
    */
    void split_labels(Label_t curr_label, std::vector<Label_t>& split_labels);

    /*!
     * Retrieve labels that have been reassigned to a given label id.
     * \param label parent label id that other labels have been reassigned to
     * \param member_labels labels that were assigned to the parent label
    */ 
    void get_label_history(Label_t label, std::vector<Label_t>& member_labels);
 
    /*!
     * Actually relabels each value in the data volume from the relabeling
     * hash table and clears the hash table.  This requires a linear-time
     * traversal of the entire volume.
    */
    void rebase_labels();

    /*!
     * Overrides the () operator defined in multiarray to return a label
     * taking into account the current label hash.
     * \param x x location
     * \param y y location
     * \param z z location
     * \return label at location
    */
    Label_t operator()(unsigned int x, unsigned int y, unsigned int z)
    {
        Label_t label = VolumeData<Label_t>::operator()(x,y,z);
        if (label_mapping.find(label) != label_mapping.end()) {
            label = label_mapping[label];
        }
        return label;
    }

    /*!
     * Set the label id at a particular location.  This calls the multiarray
     * base () operator.
     * \param x x location
     * \param y y location
     * \param z z location
    */
    void set(unsigned int x, unsigned int y, unsigned int z, Label_t val)
    {
        VolumeData<Label_t>::operator()(x,y,z) = val;
    }

    /*!
     * Retrieve all of the transform mappings since last rebasing.
     * \return copy of label mapping
    */
    std::tr1::unordered_map<Label_t, Label_t> get_mappings()
    {
        return label_mapping;
    }

  private:
    /*!
     * Private definition of constructor to prevent stack allocation.
    */
    VolumeLabelData() : VolumeData<Label_t>() {}

    //! hash table that keeps track of label mappings
    std::tr1::unordered_map<Label_t, Label_t> label_mapping;

    /*!
     * Keeps track of which labels have been mapped to which label.
     * If a lot of reassignment has occured, the reassignment procedure
     * will have runtime complexity on the order of the number of distinct
     * labels in the dataset.
    */
    std::tr1::unordered_map<Label_t, std::vector<Label_t> > label_remapping_history;

};

}

#endif
