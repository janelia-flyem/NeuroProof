#include "OrphanRank.h"
#include <Rag/RagUtils.h>

using namespace NeuroProof;

// orphan edge
void OrphanRank::initialize(double ignore_size_)
{
    ignore_size = ignore_size_;
    node_list.clear(); 

    // grab all nodes that are orphan above the size threshold
    // or that have a synapse annotation
    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        unsigned long long synapse_weight = 0;
        try {   
            synapse_weight = (*iter)->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }
        bool is_orphan = !((*iter)->is_boundary());

        if (!is_orphan || (is_orphan && (synapse_weight == 0) &&
                    ((*iter)->get_size() < ignore_size))) {
            continue;
        } 

        NodeRank body_rank;
        body_rank.id = (*iter)->get_node_id();
        body_rank.size = (*iter)->get_size();
        node_list.insert(body_rank);
    }

    update_priority();
}


RagNode_t* OrphanRank::find_most_uncertain_node(RagNode_t* head_node)
{
    AffinityPair::Hash affinity_pairs;
    grab_affinity_pairs(*rag, head_node, 0, 0.01, false, affinity_pairs);
    double biggest_change = 0;
    RagNode_t* strongest_affinity_node = 0;

    // look for the highest affinity path betwee node and non-orphan
    for (AffinityPair::Hash::iterator iter = affinity_pairs.begin();
            iter != affinity_pairs.end(); ++iter) {
        Node_t other_id = iter->region1;
        if (head_node->get_node_id() == other_id) {
            other_id = iter->region2;
        }
        RagNode_t* other_node = rag->find_rag_node(other_id);

        double local_information_affinity = -1;

        bool orphan1 = !(head_node->is_boundary());
        bool orphan2 = !(other_node->is_boundary());

        if (orphan1 && !orphan2) {
            local_information_affinity = iter->weight;;
        }

        if (local_information_affinity >= biggest_change) {
            strongest_affinity_node = rag->find_rag_node(Node_t(iter->size));
            biggest_change = local_information_affinity;
        }
    }

    return strongest_affinity_node;
}

void OrphanRank::insert_node(Node_t node)
{
    RagNode_t* head_node = rag->find_rag_node(node);
    
    NodeRank master_item;
    master_item.id = head_node->get_node_id();
    master_item.size = head_node->get_size();

    // don't insert node if not an orphan
    if (!(head_node->is_boundary())) {
        node_list.insert(master_item);
    }
}

void OrphanRank::update_neighboring_nodes(Node_t keep_node)
{
    RagNode_t* head_node = rag->find_rag_node(keep_node);
    AffinityPair::Hash affinity_pairs;
    grab_affinity_pairs(*rag, head_node, 0, 0.01, false, affinity_pairs);

    // reinsert nodes into the queue who may have been affected
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

        unsigned long long synapse_weight = 0;
        try {   
            synapse_weight = rag_other_node2->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }

        if (rag_other_node2->is_boundary() || ((item.size < ignore_size) &&
                    synapse_weight == 0)) {
            continue;
        }

        node_list.insert(item); 
    }
}
