#include "SynapseRank.h"
#include "../Rag/RagUtils.h"

using namespace NeuroProof;

void SynapseRank::initialize(double ignore_size_)
{
    ignore_size = ignore_size_;
    volume_size = 0;
    
    node_list.clear(); 

    for (Rag_uit::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        unsigned long long synapse_weight = 0;
        try {   
            synapse_weight = (*iter)->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }
        if (synapse_weight == 0) {
            continue;
        }
       
        volume_size += synapse_weight;
        NodeRank body_rank;
        body_rank.id = (*iter)->get_node_id();
        body_rank.size = synapse_weight;
        node_list.insert(body_rank);
    }
    
    voi_change_thres = calc_voi_change(ignore_size, ignore_size, volume_size);
    update_priority();
}


RagNode_uit* SynapseRank::find_most_uncertain_node(RagNode_uit* head_node)
{
    AffinityPair::Hash affinity_pairs;
    grab_affinity_pairs(*rag, head_node, 0, 0.01, true, affinity_pairs);
    double biggest_change = -1.0;
    double total_information_affinity = 0.0;
    RagNode_uit* strongest_affinity_node = 0;

    for (AffinityPair::Hash::iterator iter = affinity_pairs.begin();
            iter != affinity_pairs.end(); ++iter) {
        Node_uit other_id = iter->region1;
        if (head_node->get_node_id() == other_id) {
            other_id = iter->region2;
        }
        RagNode_uit* other_node = rag->find_rag_node(other_id);

        unsigned long long synapse_weight1 = 0;
        try {
            synapse_weight1 = head_node->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }
        unsigned long long synapse_weight2 = 0;
        try {
            synapse_weight2 = other_node->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }

        double local_information_affinity = 0;
        if (synapse_weight1 > 0 && synapse_weight2 > 0) {
            local_information_affinity = iter->weight *
                calc_voi_change(synapse_weight1, synapse_weight2, volume_size);
        }

        if (local_information_affinity >= biggest_change) {
            strongest_affinity_node = rag->find_rag_node(Node_uit(iter->size));
            biggest_change = local_information_affinity;
        }
        total_information_affinity += local_information_affinity;
    }

    if (biggest_change >= voi_change_thres) {
        return strongest_affinity_node;
    }
    return 0;
}

void SynapseRank::insert_node(Node_uit node)
{
    RagNode_uit* head_node = rag->find_rag_node(node);
    
    NodeRank master_item;
    master_item.id = head_node->get_node_id();
    master_item.size = 0;
    
    try {
        master_item.size = head_node->get_property<unsigned long long>(SynapseStr); 
    } catch(...) {

    }

    node_list.insert(master_item);
}

void SynapseRank::update_neighboring_nodes(Node_uit keep_node)
{
    RagNode_uit* head_node = rag->find_rag_node(keep_node);
    AffinityPair::Hash affinity_pairs;
    grab_affinity_pairs(*rag, head_node, 0, 0.01, true, affinity_pairs);

    for (AffinityPair::Hash::iterator iter = affinity_pairs.begin();
            iter != affinity_pairs.end(); ++iter) {
        Node_uit other_id = iter->region1;
        if (keep_node == other_id) {
            other_id = iter->region2;
        }
        RagNode_uit* rag_other_node2 = rag->find_rag_node(other_id);

        node_list.remove(other_id); 
        NodeRank item;
        item.id = other_id;
        item.size = 0;
        try { 
            item.size = rag_other_node2->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }
        if (item.size == 0) {
            continue;
        }

        node_list.insert(item); 
    }
}
