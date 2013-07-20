#ifndef FEATUREJOINALGS_H
#define FEATUREJOINALGS_H

#include "../FeatureManager/FeatureMgr.h"
#include "MergePriorityFunction.h"
#include "MergePriorityQueue.h"
#include "../Rag/RagNodeCombineAlg.h"

namespace NeuroProof {

class FeatureCombine : public RagNodeCombineAlg {
  public:
    FeatureCombine(FeatureMgr* feature_mgr_, Rag_t* rag_) :
        feature_mgr(feature_mgr_), rag(rag_) {}
    
    virtual void post_edge_move(RagEdge<unsigned int>* edge_new,
            RagEdge<unsigned int>* edge_remove)
    {
        if (feature_mgr) {
            feature_mgr->mv_features(edge_remove, edge_new);
        } 
    }

    virtual void post_edge_join(RagEdge<unsigned int>* edge_keep,
            RagEdge<unsigned int>* edge_remove)
    {
        if (feature_mgr) {
            if (edge_keep->is_false_edge()) {
                feature_mgr->mv_features(edge_remove, edge_keep); 
            } else if (!(edge_remove->is_false_edge())) {
                feature_mgr->merge_features(edge_keep, edge_remove);
            } else {
                feature_mgr->remove_edge(edge_remove);
            }
        }
    }

    virtual void post_node_join(RagNode<unsigned int>* node_keep,
            RagNode<unsigned int>* node_remove)
    {
        if (feature_mgr) {
            RagEdge_t* edge = rag->find_rag_edge(node_keep, node_remove);
            assert(edge);
            feature_mgr->merge_features(node_keep, node_remove);
            feature_mgr->remove_edge(edge);
        }
    }

  protected:
    FeatureMgr* feature_mgr;
    Rag_t* rag;
};



class DelayedPriorityCombine : public FeatureCombine {
  public:
    DelayedPriorityCombine(FeatureMgr* feature_mgr_, Rag_t* rag_, MergePriority* priority_) :
        FeatureCombine(feature_mgr_, rag_), priority(priority_) {}

    void post_node_join(RagNode<unsigned int>* node_keep,
            RagNode<unsigned int>* node_remove)
    {
        FeatureCombine::post_node_join(node_keep, node_remove);
        
        for(RagNode_t::edge_iterator iter = node_keep->edge_begin();
                iter != node_keep->edge_end(); ++iter) {
            priority->add_dirty_edge(*iter);

            RagNode_t* node = (*iter)->get_other_node(node_keep);
            for(RagNode_t::edge_iterator iter2 = node->edge_begin();
                    iter2 != node->edge_end(); ++iter2) {
                priority->add_dirty_edge(*iter2);
            }
        }
    }
  private:
    MergePriority* priority;

};

class PriorityQCombine : public FeatureCombine {
  public:
    PriorityQCombine(FeatureMgr* feature_mgr_, Rag_t* rag_,
            MergePriorityQueue<QE>* priority_) :
        FeatureCombine(feature_mgr_, rag_), priority(priority_) {}


    virtual void post_edge_move(RagEdge<unsigned int>* edge_new,
            RagEdge<unsigned int>* edge_remove)
    {
        FeatureCombine::post_edge_move(edge_new, edge_remove); 
  
        int qloc = -1;
        try {
            qloc = edge_remove->get_property<int>("qloc");
        } catch (ErrMsg& msg) {
        }

        if (qloc>=0) {
            priority->invalidate(qloc+1);
        }
    }

    virtual void post_edge_join(RagEdge<unsigned int>* edge_keep,
            RagEdge<unsigned int>* edge_remove)
    {
        FeatureCombine::post_edge_join(edge_keep, edge_remove); 
        
        int qloc = -1;
        try {
            qloc = edge_remove->get_property<int>("qloc");
        } catch (ErrMsg& msg) {
        }
        if (qloc>=0) {
            priority->invalidate(qloc+1);
        }
    }

    void post_node_join(RagNode<unsigned int>* node_keep,
            RagNode<unsigned int>* node_remove)
    {
        FeatureCombine::post_node_join(node_keep, node_remove);

        for(RagNode_t::edge_iterator iter = node_keep->edge_begin();
                iter != node_keep->edge_end(); ++iter) {
            double val = feature_mgr->get_prob(*iter);
            double prev_val = (*iter)->get_weight(); 
            (*iter)->set_weight(val);
            Node_t node1 = (*iter)->get_node1()->get_node_id();
            Node_t node2 = (*iter)->get_node2()->get_node_id();

            QE tmpelem(val, std::make_pair(node1,node2));	

            int qloc = -1;
            try {
                qloc = (*iter)->get_property<int>("qloc");
            } catch (ErrMsg& msg) {
            }

            if (qloc>=0){
                if (val<prev_val) {
                    priority->heap_decrease_key(qloc+1, tmpelem);
                } else if (val>prev_val) {	
                    priority->heap_increase_key(qloc+1, tmpelem);
                }
            } else {
                priority->heap_insert(tmpelem);
            }        

        }    
    }
  private:
    MergePriorityQueue<QE>* priority;

};

class FlatCombine : public FeatureCombine {
  public:
    FlatCombine(FeatureMgr* feature_mgr_, Rag_t* rag_,
            std::vector<QE>* priority_) :
        FeatureCombine(feature_mgr_, rag_), priority(priority_) {}


    virtual void post_edge_move(RagEdge<unsigned int>* edge_new,
            RagEdge<unsigned int>* edge_remove)
    {
        FeatureCombine::post_edge_move(edge_new, edge_remove); 
   
        edge_new->set_weight(edge_remove->get_weight());	
        
        int qloc = -1;
        try {
            qloc = edge_remove->get_property<int>("qloc");
        } catch (ErrMsg& msg) {
        }

        if (qloc>=0){
            QE tmpelem(edge_new->get_weight(),
                    std::make_pair(edge_new->get_node1()->get_node_id(), 
                    edge_new->get_node2()->get_node_id()));	
            tmpelem.reassign_qloc(qloc, FeatureCombine::rag); 	
            (*priority).at(qloc) = tmpelem; 
        }
    }

    virtual void post_edge_join(RagEdge<unsigned int>* edge_keep,
            RagEdge<unsigned int>* edge_remove)
    {
        FeatureCombine::post_edge_join(edge_keep, edge_remove); 

        // FIX?: probability calculation should be deferred to end    
        double prob = feature_mgr->get_prob(edge_keep);
        edge_keep->set_weight(prob);	

        int qloc = -1;
        try {
            qloc = edge_remove->get_property<int>("qloc");
        } catch (ErrMsg& msg) {
        }
        
        if (qloc>=0) {
            (*priority).at(qloc).invalidate();
        }
    }

  private:
    std::vector<QE>* priority;
};

}
#endif
