/*!
 * \file
 * Provides different utilities for manipulating and analyzing the Rag.
 * The utilities provided currently assumme that the unique internal
 * node identifer is of type unsigned int
*/

#ifndef RAGUTILS_H
#define RAGUTILS_H

#include <vector>

namespace boost {

template <typename T>
class shared_ptr;

}

namespace NeuroProof {

class RagNodeCombineAlg;
class OrderedPair;

template <typename Region>
class Rag;
template <typename Region>
class RagNode;
template <typename Region>
class RagEdge;

/*!
 * Function for merging node_remove onto node_keep.  The default merge operations
 * handle the node and edge size properties properly and transfer edges from
 * node_remove as appropriate.  Other user-defined properties are not explicitly
 * handled for the nodes being combined and in cases where both nodes have an
 * edge to the same body.  Handling these situtations, as well as. performing other
 * customized actions during the join can be achieved by using a combine_alg.  This
 * operation will keep node_keep's unique node identifier.
 * \param rag rag containing merged nodes
 * \param node_keep pointer to rag node whose unique identifier will be kept
 * \param node_remove pointer to rag node that will be removed
 * \param combine_alg algorithm type for performing actions during the join operation
*/
void rag_join_nodes(Rag<unsigned int>& rag, RagNode<unsigned int>* node_keep,
        RagNode<unsigned int>* node_remove, RagNodeCombineAlg* combine_alg);

/*!
 * Computes all of the biconnected components for the given graph
 * \param rag Rag used to compute bi-connected components
 * \param biconnected_components results from algorithm
*/
void find_biconnected_components(boost::shared_ptr<Rag<unsigned int> > rag,
    std::vector<std::vector<OrderedPair> >& biconnected_components);

}

#endif
