#include "NodeSizeRank.h"
#include "../Rag/RagUtils.h"
#include "../Utilities/AffinityPair.h"

using namespace NeuroProof;

void NodeSizeRank::initialize(double ignore_size_, unsigned int depth_)
{
    ignore_size = ignore_size_;
    depth = depth_;
    volume_size = rag->get_rag_size();
    // hack: BIGBODY10NM represents the 'smallest' allowable volume
    voi_change_thres = calc_voi_change(BIGBODY10NM, ignore_size, volume_size);
    node_list.clear(); 

    // add bodies above the the threshold
    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if ((*iter)->get_size() >= ignore_size) {
            NodeRank body_rank;
            body_rank.id = (*iter)->get_node_id();
            body_rank.size = (*iter)->get_size();
            node_list.insert(body_rank);
        }        
    }
    update_priority();
}

RagNode_t* NodeSizeRank::find_most_uncertain_node(RagNode_t* head_node)
{
    AffinityPair::Hash affinity_pairs;
    grab_affinity_pairs(*rag, head_node, depth, 0.01, true, affinity_pairs);
    double biggest_change = -1.0;
    double total_information_affinity = 0.0;
    RagNode_t* strongest_affinity_node = 0;

    // prioritize high affinity nodes that have large sizes
    for (AffinityPair::Hash::iterator iter = affinity_pairs.begin();
            iter != affinity_pairs.end(); ++iter) {
        Node_t other_id = iter->region1;
        if (head_node->get_node_id() == other_id) {
            other_id = iter->region2;
        }
        RagNode_t* other_node = rag->find_rag_node(other_id);

        double local_information_affinity = iter->weight *
            calc_voi_change(head_node->get_size(), other_node->get_size(), volume_size);
        if (other_node->get_size() < 1000) {
            local_information_affinity = 0.0;
        }

        if (local_information_affinity >= biggest_change) {
            strongest_affinity_node = rag->find_rag_node(Node_t(iter->size));
            biggest_change = local_information_affinity;
        }
        total_information_affinity += local_information_affinity;
    }

    if (biggest_change >= voi_change_thres) {
        return strongest_affinity_node;
    }
    return 0;
}

void NodeSizeRank::insert_node(Node_t node)
{
    RagNode_t* head_node = rag->find_rag_node(node);
    
    NodeRank master_item;
    master_item.id = head_node->get_node_id();
    master_item.size = head_node->get_size();

    node_list.insert(master_item);
}

void NodeSizeRank::update_neighboring_nodes(Node_t keep_node)
{
    RagNode_t* head_node = rag->find_rag_node(keep_node);
    AffinityPair::Hash affinity_pairs;
    grab_affinity_pairs(*rag, head_node, depth, 0.01, true, affinity_pairs);

    // reinsert all nodes that may have been affected
    // by a change in the RAG
    for (AffinityPair::Hash::iterator iter = affinity_pairs.begin();
            iter != affinity_pairs.end(); ++iter) {
        Node_t other_id = iter->region1;
        if (keep_node == other_id) {
            other_id = iter->region2;
        }
        RagNode_t* rag_other_node2 = rag->find_rag_node(other_id);

        node_list.remove(other_id); 
        NodeRank item;
        item.id = other_id;
        item.size = rag_other_node2->get_size();

        if (item.size < ignore_size) {
            continue;
        }

        node_list.insert(item); 
    }
}
