/*!
 * Defines the Stack class used as the model for storing
 * all data related to the segmentation data volume along
 * with its corresponding region adjacency graph (RAG).
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKBASE_H
#define STACKBASE_H

#include "VolumeData.h"
#include "VolumeLabelData.h"

// TODO: add forward declaration rather than including
// the entire RAG.
#include "../Rag/Rag.h"
#include "Dispatcher.h"

namespace NeuroProof {

// forward declarations of the feature manager
class FeatureMgr;
typedef boost::shared_ptr<FeatureMgr> FeatureMgrPtr;

/*!
 * The StackBase is the model that holds all information regarding
 * a segmentation (label volume, RAG, feature manager, etc).  All
 * objects in the Stack are stored as shared pointers.  Core functionality
 * should can extended through inheritence.
*/
class StackBase : public Dispatcher {
  public:
    /*!
     * Constructor for stack expects, for now, that all
     * stacks will have a label volume associated with it.
    */
    StackBase(VolumeLabelPtr labels_) : labelvol(labels_) {}

    /*!
     * Adds label volume to Stack.
     * \param labelvol_ label volume
    */    
    void set_labelvol(VolumeLabelPtr labelvol_)
    {
        labelvol = labelvol_;
    }
    
    /*!
     * Adds grayscale volume to Stack.
     * \param grayvol_ grayscale volume
    */
    void set_grayvol(VolumeGrayPtr grayvol_)
    {
        grayvol = grayvol_;
    }

    /*!
     * Adds feature manager to Stack.
     * \param feature_manager_ feature manager
    */
    void set_feature_manager(FeatureMgrPtr feature_manager_)
    {
        feature_manager = feature_manager_;
    }

    /*!
     * Adds rag to Stack.
     * \param rag_ rag
    */
    void set_rag(RagPtr rag_)
    {
        rag = rag_;
    }    

    /*!
     * Adds label volume corresponding to a ground truth to Stack.
     * \param gt_labelvol_ groundtruth label volume
    */
    void set_gt_labelvol(VolumeLabelPtr gt_labelvol_)
    {
        gt_labelvol = gt_labelvol_;
    }

    /*!
     * Adds a vector of probability volumes to Stack.
     * \param prob_list_ vector of probability volumes
    */
    void set_prob_list(std::vector<VolumeProbPtr>& prob_list_)
    {
        prob_list = prob_list_;
    }

    /*!
     * Appends probability volume to vector in Stack.
     * \param prob_ probability volume
    */
    void add_prob(VolumeProbPtr prob)
    {
        prob_list.push_back(prob);
    }

    /*!
     * Retrieve label volume from Stack.
     * \return shared pointer to label volume
    */
    VolumeLabelPtr get_labelvol()
    {
        return labelvol;
    }

    /*!
     * Retrieve grayscale volume from Stack.
     * \return shared pointer to grayscale volume
    */
    VolumeGrayPtr get_grayvol()
    {
        return grayvol;
    }

    /*!
     * Retrieve feature manager from Stack.
     * \return shared pointer to feature manager
    */
    FeatureMgrPtr get_feature_manager()
    {
        return feature_manager;
    }

    /*!
     * Retrieve rag from Stack.
     * \return shared pointer to rag
    */
    RagPtr get_rag()
    {
        return rag;
    }

    /*!
     * Retrieve groundtruth label volume from Stack.
     * \return shared pointer to groundtruth label volume
    */
    VolumeLabelPtr get_gt_labelvol()
    {
        return gt_labelvol;
    }

    /*!
     * Retrieve list of probability volumes from Stack.
     * \return vector of shared pointers to probability volumes
    */
    std::vector<VolumeProbPtr> get_prob_list()
    {
        return prob_list;
    }

  protected:
    //! label volume
    VolumeLabelPtr labelvol;

    //! grayscale corresponding to label volume
    VolumeGrayPtr grayvol;

    //! features maintained for stack corresponding to labels
    FeatureMgrPtr feature_manager;

    //! rag corresponding to the label volume
    RagPtr rag;

    // TODO: keep track of a GT "stack" intead of just the label volume
    //! ground truth label volume compared to label volume
    VolumeLabelPtr gt_labelvol;

    //! list of probability volumes used to generate features for label volume
    std::vector<VolumeProbPtr> prob_list;

    // TODO: keep track of whether stack has been modified
};

}

#endif
