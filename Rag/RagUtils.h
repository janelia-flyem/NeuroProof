#ifndef RAGUTILS_H
#define RAGUTILS_H

namespace NeuroProof {

class RagNodeCombineAlg;

template <typename Region>
class Rag;
template <typename Region>
class RagNode;
template <typename Region>
class RagEdge;

void rag_join_nodes(Rag<unsigned int>& rag, RagNode<unsigned int>* node_keep,
        RagNode<unsigned int>* node_remove, RagNodeCombineAlg* combine_alg);
}

#endif
