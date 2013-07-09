#ifndef STACK2_H
#define STACK2_H

#include "../FeatureManager/FeatureManager.h"
#include "VolumeData.h"
#include "VolumeLabelData.h"
#include "../Rag/Rag.h"

#include <vector>

namespace NeuroProof {

class Stack {
  public:
    Stack(VolumeLabelPtr labels_) : labelvol(labels_) {}
    
    void set_labelvol(VolumeLabelPtr labelvol_)
    {
        labelvol = labelvol_;
    }
    
    void set_grayvol(VolumeGrayPtr grayvol_)
    {
        grayvol = grayvol_;
    }

    void set_feature_manager(FeatureMgrPtr feature_manager_)
    {
        feature_manager = feature_manager_;
    }

    void set_rag(RagPtr rag_)
    {
        rag = rag_;
    }    

    void set_gt_labelvol(VolumeLabelPtr gt_labelvol_)
    {
        gt_labelvol = gt_labelvol_;
    }

    void set_prob_list(std::vector<VolumeProbPtr>& prob_list_)
    {
        prob_list = prob_list_;
    }

    void add_prob(VolumeProbPtr prob)
    {
        prob_list.push_back(prob);
    }

    VolumeLabelPtr get_labelvol()
    {
        return labelvol;
    }

    VolumeGrayPtr get_grayvol()
    {
        return grayvol;
    }

    FeatureMgrPtr get_feature_manager()
    {
        return feature_manager;
    }

    RagPtr get_rag()
    {
        return rag;
    }

    VolumeLabelPtr get_gt_labelvol()
    {
        return gt_labelvol;
    }

    std::vector<VolumeProbPtr> get_prob_list()
    {
        return prob_list;
    }

  private:
    VolumeLabelPtr labelvol;
    VolumeGrayPtr grayvol;
    FeatureMgrPtr feature_manager;
    RagPtr rag;

    VolumeLabelPtr gt_labelvol;
    std::vector<VolumeProbPtr> prob_list;
};


}


#endif
