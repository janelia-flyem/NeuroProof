#ifndef NODESIZERANK_H
#define NODESIZERANK_H

#include "NodeEdgeRank.h"

#include <string>

namespace NeuroProof {

class NodeSizeRank : public NodeEdgeRank {
  public:
    NodeSizeRank(Rag_uit* rag_) : NodeEdgeRank(rag_), ignore_size(BIGBODY10NM),
            voi_change_thres(0.0), depth(0), volume_size(0) {}
  
    void initialize(double ignore_size, unsigned int depth);

    RagNode_uit* find_most_uncertain_node(RagNode_uit* head_node);
    void insert_node(Node_uit node);
    void update_neighboring_nodes(Node_uit keep_node);

  private:
    double ignore_size;
    double voi_change_thres;
    unsigned int depth;
    unsigned long long volume_size;
};

}

#endif
