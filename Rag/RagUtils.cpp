/*!
 * \file
 * Implentations for different rag utilities 
*/

#include "RagUtils.h"
#include "RagNodeCombineAlg.h"
#include "Rag.h"

using namespace NeuroProof;

//TODO: create strategy for automatically merging user-defined properties
void NeuroProof::rag_join_nodes(Rag_uit& rag, RagNode_uit* node_keep, RagNode_uit* node_remove, 
        RagNodeCombineAlg* combine_alg)
{
    // iterator through all edges to be removed and transfer them or combine
    // them to the new body
    for(RagNode_uit::edge_iterator iter = node_remove->edge_begin();
            iter != node_remove->edge_end(); ++iter) {
        RagNode_uit* other_node = (*iter)->get_other_node(node_remove);
        if (other_node == node_keep) {
            continue;
        }

        // determine status of edge
        bool preserve = (*iter)->is_preserve();
        bool false_edge = (*iter)->is_false_edge();

        RagEdge_uit* final_edge = rag.find_rag_edge(node_keep, other_node);
        
        if (final_edge) {
            // merge edges -- does not merge user-defined properties by default
            preserve = preserve || final_edge->is_preserve(); 
            false_edge = false_edge && final_edge->is_false_edge(); 
            final_edge->incr_size((*iter)->get_size());

            if (combine_alg) {
                combine_alg->post_edge_join(final_edge, *iter);
            }
        } else {
            // move old edge to newly created edge
            final_edge = rag.insert_rag_edge(node_keep, other_node);
            (*iter)->mv_properties(final_edge); 
            if (combine_alg) { 
                combine_alg->post_edge_move(final_edge, *iter);
            }
        }

        final_edge->set_preserve(preserve); 
        final_edge->set_false_edge(false_edge); 
    }

    node_keep->incr_size(node_remove->get_size());
    node_keep->incr_boundary_size(node_remove->get_boundary_size());
    
    if (combine_alg) { 
        combine_alg->post_node_join(node_keep, node_remove);
    }

    // removes the node and all edges connected to it
    rag.remove_rag_node(node_remove);     
}



