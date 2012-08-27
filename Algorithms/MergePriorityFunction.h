#ifndef MERGEPRIORITYFUNCTION_H
#define MERGEPRIORITYFUNCTION_H

#include "../FeatureManager/FeatureManager.h"
#include "../DataStructures/Rag.h"

namespace NeuroProof {

class MergePriority {
  public:
    MergePriority(FeatureMgr* feature_mgr_, Rag<unsigned int>* rag_) : 
                        feature_mgr(feature_mgr_), rag(rag_) {} 

    virtual void initialize_priority(double threshold) = 0;

    virtual RagEdge<unsigned int>* get_top_edge() = 0;

    virtual void add_dirty_edge(RagEdge<unsigned int>* edge) = 0;

    bool valid_edge(RagEdge<unsigned int>* edge)
    {
        if (edge->is_preserve() || edge->is_false_edge()) {
            return false;
        }
        return true;
    }

    virtual bool empty() = 0;

  protected:
    Rag<unsigned int>* rag;
    FeatureMgr* feature_mgr;

};

class ProbPriority : public MergePriority {
  public:
    ProbPriority(FeatureMgr* feature_mgr_, Rag<unsigned int>* rag_) :
                    MergePriority(feature_mgr_, rag_), Epsilon(0.00001) {}

    void initialize_priority(double threshold_)
    {
        threshold = threshold_;
        for (Rag<unsigned int>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
            if (valid_edge(*iter)) {
                double val = feature_mgr->get_prob(*iter);

                if (val <= threshold) {
                    ranking.insert(std::make_pair(val, std::make_pair((*iter)->get_node1()->get_node_id(), (*iter)->get_node2()->get_node_id())));
                }
            }
        }
    }

    bool empty()
    {
        return ranking.empty();
    }


    RagEdge<unsigned int>* get_top_edge()
    {
        EdgeRank_t::iterator first_entry = ranking.begin();
        double curr_threshold = (*first_entry).first;
        Label node1 = (*first_entry).second.first;
        Label node2 = (*first_entry).second.second;
        ranking.erase(first_entry);

        //cout << curr_threshold << " " << node1 << " " << node2 << std::endl;

        if (curr_threshold > threshold) {
            ranking.clear();
            return 0;
        }

        RagNode<Label>* rag_node1 = rag->find_rag_node(node1); 
        RagNode<Label>* rag_node2 = rag->find_rag_node(node2); 

        if (!(rag_node1 && rag_node2)) {
            return 0;
        }
        RagEdge<Label>* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

        if (!rag_edge) {
            return 0;
        }

        if (!valid_edge(rag_edge)) {
            return 0;
        }

        double val = feature_mgr->get_prob(rag_edge);
        if (val > (curr_threshold + Epsilon)) {
            return 0;
        }
        return rag_edge; 
    }

    void add_dirty_edge(RagEdge<unsigned int>* edge)
    {
        if (valid_edge(edge)) {
            edge->set_dirty(true);
            double val = feature_mgr->get_prob(edge);
            ranking.insert(std::make_pair(val, std::make_pair((edge)->get_node1()->get_node_id(), (edge)->get_node2()->get_node_id())));
            edge->set_dirty(false);
        }
    }

  private:

    double threshold;
    const double Epsilon;
    typedef std::multimap<double, std::pair<unsigned int, unsigned int> > EdgeRank_t; 
    EdgeRank_t ranking;

};

}

#endif
