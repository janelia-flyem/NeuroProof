#ifndef RAGNODECOMBINEALG_H
#define RAGNODECOMBINEALG_H

namespace NeuroProof {

template <typename Region>
class RagNode;

template <typename Region>
class RagEdge;

class RagNodeCombineAlg {
  public:
    virtual void post_edge_move(RagEdge<unsigned int>* edge_new,
            RagEdge<unsigned int>* edge_remove) = 0;

    virtual void post_edge_join(RagEdge<unsigned int>* edge_keep,
            RagEdge<unsigned int>* edge_remove) = 0;

    virtual void post_node_join(RagNode<unsigned int>* node_keep,
            RagNode<unsigned int>* node_remove) = 0;

    virtual ~RagNodeCombineAlg() {}
};

}

#endif


