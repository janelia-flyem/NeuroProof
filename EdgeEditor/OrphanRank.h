#ifndef ORPHANRANK_H
#define ORPHANRANK_H

#include "NodeEdgeRank.h"

#include <string>

namespace NeuroProof {

class OrphanRank : public NodeEdgeRank {
  public:
    OrphanRank(Rag_uit* rag_) : NodeEdgeRank(rag_),
        SynapseStr("synapse_weight"), ignore_size(BIGBODY10NM) {}
  
    void initialize(double ignore_size);

    RagNode_uit* find_most_uncertain_node(RagNode_uit* head_node);
    void insert_node(Node_uit node);
    void update_neighboring_nodes(Node_uit keep_node);

  private:
    const std::string SynapseStr;
    double ignore_size;
};

}

#endif
