/*!
 * \file
 * Implentations for different rag utilities 
*/

#include "RagUtils.h"
#include "RagNodeCombineAlg.h"
#include "Rag.h"

#include <boost/shared_ptr.hpp>
#include <tr1/unordered_set>
#include <tr1/unordered_map>

#include <boost/graph/graph_traits.hpp>
#include <queue>

using std::vector;
using std::tr1::unordered_map;
using std::tr1::unordered_set;

namespace NeuroProof {

//TODO: create strategy for automatically merging user-defined properties
void rag_join_nodes(Rag_t& rag, RagNode_t* node_keep, RagNode_t* node_remove, 
        RagNodeCombineAlg* combine_alg)
{
    // iterator through all edges to be removed and transfer them or combine
    // them to the new body
    for(RagNode_t::edge_iterator iter = node_remove->edge_begin();
            iter != node_remove->edge_end(); ++iter) {
        RagNode_t* other_node = (*iter)->get_other_node(node_remove);
        if (other_node == node_keep) {
            continue;
        }

        // determine status of edge
        bool preserve = (*iter)->is_preserve();
        bool false_edge = (*iter)->is_false_edge();

        RagEdge_t* final_edge = rag.find_rag_edge(node_keep, other_node);
        
        if (final_edge) {
            // merge edges -- does not merge user-defined properties by default
            preserve = preserve || final_edge->is_preserve(); 
            false_edge = false_edge && final_edge->is_false_edge(); 
            final_edge->incr_size((*iter)->get_size());
            if (combine_alg) {
                combine_alg->post_edge_join(final_edge, *iter);
            }

            // specific flag updates for a particular algorithm, will be ignored
            // if these flags do not exist
            try {
                double prob1 = (*iter)->get_property<double>("orig-prob");
                double prob2 = final_edge->get_property<double>("orig-prob");
                final_edge->set_property("orig-prob", double(std::min(prob1, prob2)));
                prob1 = (*iter)->get_property<double>("save-prob");
                prob2 = final_edge->get_property<double>("save-prob");
                final_edge->set_property("save-prob", double(std::min(prob1, prob2)));
            } catch (ErrMsg& msg) {
            }

        } else {
            // move old edge to newly created edge
            final_edge = rag.insert_rag_edge(node_keep, other_node);
            (*iter)->mv_properties(final_edge); 
            final_edge->set_size((*iter)->get_size());
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

/*!
 * Structure used in bi-connected computation
*/
struct DFSNode {
    Node_t previous;
    RagNode_t* rag_node;  
    int count;
    int start_pos;
};

/*! 
 * Contains parameters needed to compute biconnected components
*/
struct BiconnectedParams {
    BiconnectedParams(RagPtr rag_, vector<vector<OrderedPair> >&
        biconnected_components_, vector<DFSNode>& dfs_stack_) : rag(rag_),
        biconnected_components(biconnected_components_), dfs_stack(dfs_stack_) {}  

    //! bi-connected components computed for rag
    RagPtr rag;

    //! bi-connected component output
    vector<vector<OrderedPair> >& biconnected_components;

    //! dfs_stack that contains initial boundary node as starting point
    vector<DFSNode>& dfs_stack;
    
    //! internal variable to biconnected calculation
    unordered_set<Node_t> visited;

    //! internal variable to biconnected calculation
    unordered_map<Node_t, int> node_depth;

    //! internal variable to biconnected calculation
    unordered_map<Node_t, int> low_count;

    //! internal variable to biconnected calculation
    unordered_map<Node_t, Node_t> prev_id;
    
    //! internal variable to biconnected calculation
    vector<OrderedPair> stack;
};

void biconnected_recurs(BiconnectedParams& params)
{
    while (!params.dfs_stack.empty()) {
        DFSNode entry = params.dfs_stack.back();
        RagNode_t* rag_node = entry.rag_node;
        Node_t previous = entry.previous;
        int count = entry.count;
        params.dfs_stack.pop_back();

        if (params.visited.find(rag_node->get_node_id()) == params.visited.end()) {
            params.visited.insert(rag_node->get_node_id());
            params.node_depth[rag_node->get_node_id()] = count;
            params.low_count[rag_node->get_node_id()] = count;
            params.prev_id[rag_node->get_node_id()] = previous;
        }

        bool skip = false;
        int curr_pos = 0;
        for (RagNode_t::node_iterator iter = rag_node->node_begin();
                iter != rag_node->node_end(); ++iter) {
            RagEdge_t* rag_edge = params.rag->find_rag_edge(rag_node, *iter);
            if (rag_edge->is_false_edge()) {
                continue;
            }

            if (curr_pos < entry.start_pos) {
                ++curr_pos;
                continue;
            }
            if (params.prev_id[(*iter)->get_node_id()] == rag_node->get_node_id()) {
                OrderedPair current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
                int temp_low = params.low_count[(*iter)->get_node_id()];
                params.low_count[rag_node->get_node_id()] =
                    std::min(params.low_count[rag_node->get_node_id()], temp_low);

                if (temp_low >= count) {
                    OrderedPair popped_edge;
                    params.biconnected_components.push_back(std::vector<OrderedPair>());
                    do {
                        popped_edge = params.stack.back();
                        params.stack.pop_back();
                        params.biconnected_components[params.biconnected_components.size()-1].push_back(popped_edge);
                    } while (!(popped_edge == current_edge));
                    OrderedPair articulation_pair(rag_node->get_node_id(), rag_node->get_node_id());
                    params.biconnected_components[params.biconnected_components.size()-1].push_back(articulation_pair);
                } 
            } else if (params.visited.find((*iter)->get_node_id()) == params.visited.end()) {
                OrderedPair current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
                params.stack.push_back(current_edge);

                DFSNode temp;
                temp.previous = rag_node->get_node_id();
                temp.rag_node = (*iter);
                temp.count = count+1;
                temp.start_pos = 0;
                entry.start_pos = curr_pos;
                params.dfs_stack.push_back(entry);
                params.dfs_stack.push_back(temp);
                skip = true;
                break;
            } else if ((*iter)->get_node_id() != previous) {
                params.low_count[rag_node->get_node_id()] = std::min(
                    params.low_count[rag_node->get_node_id()],
                    params.node_depth[(*iter)->get_node_id()]);
                if (count > params.node_depth[(*iter)->get_node_id()]) {
                    params.stack.push_back(OrderedPair(rag_node->get_node_id(),
                        (*iter)->get_node_id()));
                }
            }
            ++curr_pos;
        }

        if (skip) {
            continue;
        }

        bool border = rag_node->is_boundary();
        if (previous && border) {
            params.low_count[rag_node->get_node_id()] = 0;
            params.stack.push_back(OrderedPair(0, rag_node->get_node_id()));
        }
    }
}

void find_biconnected_components(RagPtr rag, vector<vector<OrderedPair> >& biconnected_components)
{
    RagNode_t* rag_node = 0;
    for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        if ((*iter)->is_boundary()) {
            rag_node = *iter;
            break;
        }
    }
    assert(rag_node);

    DFSNode temp;
    temp.previous = 0;
    temp.rag_node = rag_node;
    temp.count = 1;
    temp.start_pos = 0;

    vector<DFSNode> dfs_stack;
    dfs_stack.push_back(temp);

    BiconnectedParams params(rag, biconnected_components, dfs_stack);

    biconnected_recurs(params);
}

void compute_graph_coloring(boost::shared_ptr<Rag<Index_t> > rag)
{
    unordered_set<int> used_ids;
    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        if (!((*iter)->has_property("color"))) {
            used_ids.clear();
            for (RagNode_t::node_iterator iter2 = (*iter)->node_begin();
                    iter2 != (*iter)->node_end(); ++iter2) {
                int color_id = -1;
                try {
                    color_id = (*iter2)->get_property<int>("color");
                    used_ids.insert(color_id);
                } catch (ErrMsg& msg) {
                    //
                }            
            }
            int color_id = 0;
            while (used_ids.find(color_id) != used_ids.end()) ++color_id;
            (*iter)->set_property("color", color_id);
        }
    }
}

BoostGraph* create_boost_graph(RagPtr rag)
{
    BoostGraph* graph = new BoostGraph;

    for (Rag_t::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter) {
        BoostEdgeBool edge = boost::add_edge((*iter)->get_node1()->get_node_id(),
                (*iter)->get_node2()->get_node_id(), *graph);

        // add edge properties       
        (*graph)[edge.first].size = (*iter)->get_size();
        (*graph)[edge.first].weight = (*iter)->get_weight();
    }

    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        // add vertex properties
        (*graph)[((*iter)->get_node_id())].size = (*iter)->get_size();
        (*graph)[((*iter)->get_node_id())].boundary_size =
            (*iter)->get_boundary_size();
    }

    /*
     To get both vertices corresponding to a given edge, use
     boost::target(BoostEdge& edge) and boost::source(BoostEdge& edge).
     The following code can be used to traverse the graph vertices:
     
     std::pair<Graph::vertex_iterator, Graph::vertex_iterator>
        vertexIteratorRange = boost::vertices(*graph);
     for(Graph::vertex_iterator vertexIterator = vertexIteratorRange.first;
        vertexIterator != vertexIteratorRange.second; ++vertexIterator)

     Graph edges can be traversed by replacing 'vertex' with 'edge'
    */

    return graph;
}

/*!
 * Structure used in the Dijkstra's algorithm implementation of finding
 * the shortest multiplicative path in the graph.
*/
struct BestNode {
    RagNode_t* rag_node_curr;
    RagEdge_t* rag_edge_curr;
    //! weight of the current path (1 is short, 0 is infinite)
    double weight;
    //! length of the current path
    int path;
    Node_t second_node;
};
struct BestNodeCmp {
    bool operator()(const BestNode& q1, const BestNode& q2) const
    {
        return (q1.weight < q2.weight);
    }
};

void grab_affinity_pairs(Rag_t& rag, RagNode_t* rag_node_head, int path_restriction,
        double connection_threshold, bool preserve, AffinityPair::Hash& affinity_pairs)
{
    typedef std::priority_queue<BestNode, std::vector<BestNode>, BestNodeCmp> BestNodeQueue;
    BestNode best_node_head;
    BestNodeQueue best_node_queue; 
    
    best_node_head.rag_node_curr = rag_node_head;
    best_node_head.rag_edge_curr = 0;
    best_node_head.weight= 1.0;
    best_node_head.path = 0;
    Node_t node_head = rag_node_head->get_node_id();

    best_node_queue.push(best_node_head);
    AffinityPair affinity_pair_head(node_head, node_head);
    affinity_pair_head.weight = 1.0;
    affinity_pair_head.size = 0;

    affinity_pairs.clear();
    
    // finding the shortest current path (connection strenght closest to 1
    // and pop this value off the list)
    while (!best_node_queue.empty()) {
        BestNode best_node_curr = best_node_queue.top();
        AffinityPair affinity_pair_curr(node_head, best_node_curr.rag_node_curr->get_node_id());

        if (affinity_pairs.find(affinity_pair_curr) == affinity_pairs.end()) { 
            for (RagNode_t::edge_iterator edge_iter = best_node_curr.rag_node_curr->edge_begin();
                    edge_iter != best_node_curr.rag_node_curr->edge_end(); ++edge_iter) {
                // avoid simple cycles
                if (*edge_iter == best_node_curr.rag_edge_curr) {
                    continue;
                }

                // grab other node 
                RagNode_t* other_node = (*edge_iter)->get_other_node(best_node_curr.rag_node_curr);

                // avoid duplicates
                AffinityPair temp_pair(node_head, other_node->get_node_id());
                if (affinity_pairs.find(temp_pair) != affinity_pairs.end()) {
                    continue;
                }

                if (path_restriction && (best_node_curr.path == path_restriction)) {
                    continue;
                }

                RagEdge_t* rag_edge_temp = rag.find_rag_edge(rag_node_head, other_node); 
                if (rag_edge_temp && rag_edge_temp->get_weight() > 1.00001) {
                    continue;
                }

                if (preserve) {
                    if ((rag_edge_temp && rag_edge_temp->is_preserve()) || (!rag_edge_temp && ((*edge_iter)->is_preserve()))) {
                        continue;
                    }
                }

                if (rag_edge_temp && rag_edge_temp->is_false_edge()) {
                    rag_edge_temp = 0;
                }

                double edge_prob = 1.0 - (*edge_iter)->get_weight();

                if (edge_prob < 0.000001) {
                    continue;
                }

                edge_prob = best_node_curr.weight * edge_prob;
                if (edge_prob < connection_threshold) {
                    continue;
                }

                BestNode best_node_new;
                best_node_new.rag_node_curr = other_node;
                best_node_new.rag_edge_curr = *edge_iter;
                best_node_new.weight = edge_prob;
                best_node_new.path = best_node_curr.path + 1;
                if (best_node_new.path > 1) {
                    best_node_new.second_node = best_node_curr.second_node;
                } else {
                    best_node_new.second_node = best_node_new.rag_node_curr->get_node_id();
                }
                if (rag_edge_temp) {
                    best_node_new.second_node = best_node_new.rag_node_curr->get_node_id();
                }

                best_node_queue.push(best_node_new);
            }
            affinity_pair_curr.weight = best_node_curr.weight;
            if (best_node_curr.path >= 1) {
                affinity_pair_curr.size = best_node_curr.second_node;
            }
            affinity_pairs.insert(affinity_pair_curr);
        }

        best_node_queue.pop();
    }

    affinity_pairs.erase(affinity_pair_head);
}

double find_affinity_path(Rag_t& rag, RagNode_t* rag_node_head, RagNode_t* rag_node_dest)
{
    // TODO restrict path search algorithm so that it terminates after
    // finding the destination node
    AffinityPair::Hash affinity_pairs;
    
    // current ignore preserve nodes 
    grab_affinity_pairs(rag, rag_node_head, 0, 0.01, false, affinity_pairs); 
    AffinityPair apair(rag_node_head->get_node_id(), rag_node_dest->get_node_id()); 

    AffinityPair::Hash::iterator iter = affinity_pairs.find(apair);
    if (iter == affinity_pairs.end()) {
        return 0.0;
    } else {
        return iter->weight;
    }
    //return int(-1*log(iter->weight)/log(2.0)+0.5);
}

}
