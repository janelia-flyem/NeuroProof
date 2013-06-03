#include "RagUtils.h"
#include "RagNodeCombineAlg.h"

using namespace NeuroProof;


// missing merge rules (including with edge weight -- could make property) -- could make handlers
// missing tie with feature merger -- will need to stop increasing node and edge size
void NeuroProof::rag_join_nodes(Rag_uit& rag, RagNode_uit* node_keep, RagNode_uit* node_remove, 
        RagNodeCombineAlg* combine_alg)
{
    RagEdge_uit* edge = rag.find_rag_edge(node_keep, node_remove);
    for(RagNode_uit::edge_iterator iter = node_remove->edge_begin();
            iter != node_remove->edge_end(); ++iter) {
        RagNode_uit* other_node = (*iter)->get_other_node(node_remove);
        if (other_node == node_keep) {
            continue;
        }
        bool preserve = (*iter)->is_preserve();
        bool false_edge = (*iter)->is_false_edge();

        RagEdge_uit* final_edge = rag.find_rag_edge(node_keep, other_node);
        if (final_edge) {
            preserve = preserve || final_edge->is_preserve(); 
            false_edge = false_edge && final_edge->is_false_edge(); 
            final_edge->incr_size((*iter)->get_size());

            combine_alg->post_edge_join(final_edge, *iter);
        } else {
            final_edge = rag.insert_rag_edge(node_keep, other_node);
            (*iter)->mv_properties(final_edge); 
            
            combine_alg->post_edge_move(final_edge, *iter);
        }

        final_edge->set_preserve(preserve); 
        final_edge->set_false_edge(false_edge); 
    }

    node_keep->incr_size(node_remove->get_size());
    node_keep->incr_boundary_size(node_remove->get_boundary_size());
     
    combine_alg->post_node_join(node_keep, node_remove);
    rag.remove_rag_node(node_remove);     
}



