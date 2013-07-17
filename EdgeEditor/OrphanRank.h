#ifndef ORPHANRANK_H
#define ORPHANRANK_H

#include "NodeCentricRank.h"

#include <string>

namespace NeuroProof {

class OrphanRank : public NodeCentricRank {
  public:
    OrphanRank(Rag_uit* rag_) : NodeCentricRank(rag_),
        SynapseStr("synapse_weight"), ignore_size(BIGBODY10NM) {}
  
    void initialize(double ignore_size);

    RagNode_uit* find_most_uncertain_node(RagNode_uit* head_node);
    void insert_node(Node_uit node);
    void update_neighboring_nodes(Node_uit keep_node);
    std::string get_identifier()
    {
        return std::string("orphan");
    }


  private:
    const std::string SynapseStr;
    double ignore_size;
};

}

#endif
