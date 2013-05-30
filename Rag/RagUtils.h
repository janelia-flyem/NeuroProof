#ifndef RAGUTILS_H
#define RAGUTILS_H

#include "Rag.h"

#include <map>
#include <string>
#include <iostream>

namespace NeuroProof {


// take smallest value edge and use that when connecting back
void rag_merge_edge(Rag_uit& rag, RagEdge_uit* edge, RagNode_uit* node_keep, std::vector<std::string>& property_names);

}

#endif
