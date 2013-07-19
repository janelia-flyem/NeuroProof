#include "ProbEdgeRank.h"

using namespace NeuroProof;

void ProbEdgeRank::examined_edge(NodePair node_pair, bool remove)
{
    ++num_processed;
    if (!remove) {
        RagEdge_uit* edge = rag->find_rag_edge(boost::get<0>(node_pair),
                boost::get<1>(node_pair));

        EdgeRanking::iterator iter;
        for (iter = edge_ranking.begin(); iter != edge_ranking.end(); ++iter) {
            if (iter->second == edge) {
                break;
            }
        }
        if (iter != edge_ranking.end()) {
            edge_ranking.erase(iter);
        }
    } else {
        update_priority();
    } 
}

void ProbEdgeRank::grab_edge_ranking()
{
    edge_ranking.clear();

    // iterate all edges and add those in a specified range
    for (Rag_uit::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        double val = (*iter)->get_weight();
        if ((val <= upper) && (val >= lower)) {
            if (((*iter)->get_node1()->get_size() > ignore_size) &&
                    ((*iter)->get_node2()->get_size() > ignore_size)) {
                edge_ranking.insert(std::make_pair(std::abs(val - start), *iter));
            }
        } 
    }
}

bool ProbEdgeRank::get_top_edge(NodePair& top_edge_ret)
{
    if (edge_ranking.empty()) {
        return false;
    }
    RagEdge_uit* edge = edge_ranking.begin()->second;
    RagNode_uit* node1 = edge->get_node1();
    RagNode_uit* node2 = edge->get_node2();

    if (node1->get_size() >= node2->get_size()) {
        top_edge_ret = NodePair(node1->get_node_id(), node2->get_node_id());
    } else {
        top_edge_ret = NodePair(node2->get_node_id(), node1->get_node_id());
    }

    return true; 
}

void ProbEdgeRank::undo()
{
    --num_processed;
    update_priority();
}

void ProbEdgeRank::update_priority()
{
    grab_edge_ranking();
}

unsigned int ProbEdgeRank::get_num_remaining()
{
    return edge_ranking.size();
} 

bool ProbEdgeRank::is_finished()
{
    return edge_ranking.empty();
} 

void ProbEdgeRank::initialize(double lower_, double upper_, double start_,
        double ignore_size_)
{
    lower = lower_;
    upper = upper_;
    start = start_;
    ignore_size = ignore_size_;
   
    update_priority();
}
