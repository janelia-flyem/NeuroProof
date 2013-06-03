#ifndef RAGUTILS_H
#define RAGUTILS_H

#include "Rag.h"

#include <map>
#include <string>
#include <iostream>

namespace NeuroProof {

class RagNodeCombineAlg;

void rag_join_nodes(Rag_uit& rag, RagNode_uit* node_keep, RagNode_uit* node_remove, 
        RagNodeCombineAlg* combine_alg);
}

#endif
