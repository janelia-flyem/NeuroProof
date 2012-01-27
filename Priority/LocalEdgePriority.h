#ifndef LOCALEDGEPRIORITY_H
#define LOCALEDGEPRIORITY_H

#include "EdgePriority.h"
#include "../Algorithms/RagAlgs.h"
#include <iostream>

namespace NeuroProof {

template <typename Region>
class LocalEdgePriority : public EdgePriority<Region> {
  public:
    typedef boost::tuple<Region, Region> NodePair;
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;
    LocalEdgePriority(Rag<Region>& rag_, double min_val_, double max_val_, double start_val_) 
        : EdgePriority<Region>(rag_), min_val(min_val_), max_val(max_val_), start_val(start_val_)
    {


        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        for (typename Rag<Region>::edges_iterator iter = ragtemp.edges_begin();
                iter != ragtemp.edges_end(); ++iter) {
            if ((rag_retrieve_property<Region, unsigned int>(&ragtemp, *iter, "edge_size") <= 1)) {
                (*iter)->set_weight(2.0);
            }
        } 


#if 0    
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        std::vector<RagEdge<Region>* > remove_edges;
        for (typename Rag<Region>::edges_iterator iter = ragtemp.edges_begin();
                iter != ragtemp.edges_end(); ++iter) {
            if ((rag_retrieve_property<Region, unsigned int>(&ragtemp, *iter, "edge_size") <= 1)
                || (((*iter)->get_weight() > max_val) || ((*iter)->get_weight() < min_val))  ) {
                remove_edges.push_back(*iter);         
            }
        } 
        for (int i = 0; i < remove_edges.size(); ++i) {
            ragtemp.remove_rag_edge(remove_edges[i]);
        }
        

        std::vector<RagNode<Region>* > remove_nodes;
        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin();
                iter != ragtemp.nodes_end(); ++iter) {
            if ((*iter)->get_size() <= 27) {
                remove_nodes.push_back(*iter);
            }
        }
        for (int i = 0; i < remove_nodes.size(); ++i) {
            ragtemp.remove_rag_node(remove_nodes[i]);
        }
#endif
        //EdgePriority<Region>::rag.unbind_property_list("edge_size");
    }
    NodePair getTopEdge(Location& location);
    void updatePriority();
    bool isFinished(); 
    void setEdge(NodePair node_pair, double weight);
    unsigned int getNumRemaining() const;
    bool undo();
    void removeEdge(NodePair node_pair, bool remove);

  private:
    typename EdgeRanking<Region>::type edge_ranking;
    double min_val, max_val, start_val; 
};

// -algorithms-
// sort_weight
// sort_property(rag, property_string) -- throw on error
// get_top_weight(rag)
// get_top_property(rag, property_string) -- throw on error
// set_property(rag, property_string)
// set_property_edge(rag, edge, property_string, property)
// retrieve_property_edge(rag, edge, property_string)
// serialize and deserialize?
// remove_property(rag, property_string)
// remove_property_edge(rag, edge, property_string)
// delete edge should be here??
//
// -rag-
// hash of edges with hash of properties -- fill as used
// hash of vector<Property> -- slots added for each sort
// automatic deletion and copying of data structures

template <typename Region> unsigned int LocalEdgePriority<Region>::getNumRemaining() const
{
    return (unsigned int)(edge_ranking.size());
}

template <typename Region> void LocalEdgePriority<Region>::updatePriority()
{
    edge_ranking = rag_grab_edge_ranking(EdgePriority<Region>::rag, min_val, max_val, start_val);
    EdgePriority<Region>::setUpdated(true);
}

template <typename Region> bool LocalEdgePriority<Region>::isFinished()
{
    return (EdgePriority<Region>::isUpdated() && edge_ranking.empty());
}

template <typename Region> void LocalEdgePriority<Region>::setEdge(NodePair node_pair, double weight)
{
    RagEdge<Region>* edge = (EdgePriority<Region>::rag).find_rag_edge(boost::get<0>(node_pair), boost::get<1>(node_pair));
    typename EdgeRanking<Region>::type::iterator iter;
    for (iter = edge_ranking.begin(); iter != edge_ranking.end(); ++iter) {
        if (iter->second == edge) {
            break;
        }
    }
    if (iter != edge_ranking.end()) {
        edge_ranking.erase(iter);
    }
    EdgePriority<Region>::setEdge(node_pair, weight);
}

template <typename Region> bool LocalEdgePriority<Region>::undo()
{
    bool ret = EdgePriority<Region>::undo();
    updatePriority();
    return ret; 
}

template <typename Region> void LocalEdgePriority<Region>::removeEdge(NodePair node_pair, bool remove)
{
    if (remove) {
        EdgePriority<Region>::removeEdge(node_pair, remove);
        updatePriority();    
    } else {
        setEdge(node_pair, 1.0);
    }
}

template <typename Region> boost::tuple<Region, Region> LocalEdgePriority<Region>::getTopEdge(Location& location)
{
    RagEdge<Region>* edge;
    try {
        if (!EdgePriority<Region>::isUpdated()) {
            updatePriority();
        }
        if (!edge_ranking.empty()) {
            edge = edge_ranking.begin()->second;
        } else {
            throw ErrMsg("Priority queue is empty");
        }
       
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        location = rag_retrieve_property<Region, Location>(&ragtemp, edge, "location");
    } catch(ErrMsg& msg) {
        std::cerr << msg.str << std::endl;
        throw ErrMsg("Priority scheduler crashed");
    }

    if (edge->get_node1()->get_size() >= edge->get_node2()->get_size()) {
        return NodePair(edge->get_node1()->get_node_id(), edge->get_node2()->get_node_id());
    } else {
        return NodePair(edge->get_node2()->get_node_id(), edge->get_node1()->get_node_id());
    }
}


}

#endif
