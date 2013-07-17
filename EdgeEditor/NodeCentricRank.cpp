#include "NodeCentricRank.h"
#include <cmath>

using std::vector;

namespace NeuroProof {

void NodeRankList::insert(NodeRank item)
{
    if (checkpoint) {
        history[history.size()-1].push_back(HistoryElement(item, true));
    }
    node_list.insert(item);
    stored_ids[item.id] = item;
}

void NodeRankList::clear()
{
    node_list.clear();
    stored_ids.clear();
    checkpoint = false;
    history.clear();
}

bool NodeRankList::empty()
{
    return node_list.empty();
}

size_t NodeRankList::size()
{
    return node_list.size();
}

NodeRank NodeRankList::first()
{
    return *(node_list.begin());
}

void NodeRankList::pop()
{
    if (checkpoint) {
        history[history.size()-1].push_back(HistoryElement(first(), false));
    }
    Node_uit head_id = first().id;
    stored_ids.erase(head_id);
    node_list.erase(node_list.begin());
}

void NodeRankList::remove(Node_uit id)
{
    if (stored_ids.find(id) == stored_ids.end()) {
        return;
    }

    if (checkpoint) {
        history[history.size()-1].push_back(HistoryElement(stored_ids[id], false));
    }

    node_list.erase(stored_ids[id]);
    stored_ids.erase(id);
}

void NodeRankList::remove(NodeRank item)
{
    if (checkpoint) {
        history[history.size()-1].push_back(HistoryElement(stored_ids[item.id], false));
    }

    node_list.erase(item);
    stored_ids.erase(item.id);
}

void NodeRankList::start_checkpoint()
{
    checkpoint = true;
    history.push_back(vector<HistoryElement>());
}

void NodeRankList::stop_checkpoint()
{
    checkpoint = false;
}

void NodeRankList::undo_one()
{
    int num_checkpoints = history.size()-1;
    int num_actions = history[num_checkpoints].size();

    for (int i = num_actions - 1; i >= 0; --i) {
        HistoryElement entry = history[num_checkpoints][i];
        if (entry.inserted_node) {
            remove(entry.item);
        } else {
            insert(entry.item);
        }
    }

    history.pop_back(); 
}




bool NodeCentricRank::get_top_edge(NodePair& top_edge_ret)
{
    if ((boost::get<0>(top_edge) == 0) && (boost::get<1>(top_edge) == 0)) {
        assert(node_list.empty());
        return false;
    }

    top_edge_ret = top_edge;
    return true;
}

void NodeCentricRank::update_priority()
{
    top_edge = NodePair(0,0);
    while (!node_list.empty()) {
        Node_uit head_id = node_list.first().id; 

        RagNode_uit* head_node = rag->find_rag_node(head_id);

        // going Hollywood
        RagNode_uit* strongest_affinity_node =
            find_most_uncertain_node(head_node);        

        if (strongest_affinity_node) {
            if (strongest_affinity_node->get_size() >= head_node->get_size()) {
                top_edge = NodePair(strongest_affinity_node->get_node_id(),
                        head_node->get_node_id());
            } else {
                top_edge = NodePair(head_node->get_node_id(),
                        strongest_affinity_node->get_node_id());
            }
            break;
        } else {
            node_list.pop();
        }
    }
}

void NodeCentricRank::undo()
{
    --num_processed;
    node_list.undo_one();
    update_priority();
}

// called after node merge
void NodeCentricRank::examined_edge(NodePair node_pair, bool remove)
{
    ++num_processed;
    Node_uit keep_node = boost::get<0>(node_pair);
    node_list.start_checkpoint();
    
    if (remove) {
        node_list.remove(boost::get<1>(node_pair));
        node_list.remove(keep_node);
        
        // virtual call to add node if it still qualifies
        insert_node(keep_node);
        // potentially put neighboring nodes back in body list
        // virtual function call
        update_neighboring_nodes(keep_node);
    }
    
    update_priority();
    node_list.stop_checkpoint();
}

double NodeCentricRank::calc_voi_change(double size1, double size2,
        unsigned long long total_volume)
{
    double part1 = log(size1/total_volume)/log(2.0)*size1/total_volume + 
        log(size2/total_volume)/log(2.0)*size2/total_volume;
    double part2 = log((size1 + size2)/total_volume)/
                    log(2.0)*(size1 + size2)/total_volume;
    double part3 = -1 * (part1 - part2);
    return part3;
}

}
