#ifndef LOCALEDGEPRIORITY_H
#define LOCALEDGEPRIORITY_H

#include "EdgePriority.h"
#include "../Algorithms/RagAlgs.h"
#include <iostream>
#include <json/value.h>
#include <set>

namespace NeuroProof {

template <typename Region>
class LocalEdgePriority : public EdgePriority<Region> {
  public:
    typedef boost::tuple<Region, Region> NodePair;
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;
    
    LocalEdgePriority(Rag<Region>& rag_, double min_val_, double max_val_, double start_val_) 
        : EdgePriority<Region>(rag_), min_val(min_val_), max_val(max_val_), start_val(start_val_), num_processed(0), num_correct(0), cum_error(0.0), prune_small_edges(true), ignore_size(27), body_mode(false)
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        for (typename Rag<Region>::edges_iterator iter = ragtemp.edges_begin();
                iter != ragtemp.edges_end(); ++iter) {
            if ((rag_retrieve_property<Region, unsigned int>(&ragtemp, *iter, "edge_size") <= 1)) {
                (*iter)->set_weight(10.0);
            }
        } 
    }
    LocalEdgePriority(Rag<Region>& rag_, double min_val_, double max_val_, double start_val_, Json::Value& json_vals) 
        : EdgePriority<Region>(rag_), min_val(min_val_), max_val(max_val_), start_val(start_val_), num_processed(0), num_correct(0), cum_error(0.0), prune_small_edges(true), ignore_size(27), body_mode(false)
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        
        prune_small_edges = json_vals.get("prune_small_edges", true).asBool(); 
        if (prune_small_edges) {
            for (typename Rag<Region>::edges_iterator iter = ragtemp.edges_begin();
                    iter != ragtemp.edges_end(); ++iter) {
                if ((rag_retrieve_property<Region, unsigned int>(&ragtemp, *iter, "edge_size") <= 1)) {
                    (*iter)->set_weight(10.0);
                }
            }
        } 

        Json::Value json_range = json_vals["range"];
        if (!json_range.empty()) {
            min_val = json_range[(unsigned int)(0)].asDouble();
            max_val = json_range[(unsigned int)(1)].asDouble();
            start_val = min_val;
        }

        num_processed = json_vals.get("num_processed", 0).asUInt(); 
        num_correct = json_vals.get("num_correct", 0).asUInt(); 
        cum_error = json_vals.get("cum_error", 0.0).asDouble(); 
        ignore_size = json_vals.get("ignore_size", 27).asUInt();
    
        body_mode = json_vals.get("body_mode", false).asBool(); 

        Json::Value json_reexamine = json_vals["reexamine_bodies"];
        for (unsigned int i = 0; i < json_reexamine.size(); ++i) {
            BodyRank body_rank;
            body_rank.id = json_reexamine[i].asUInt();
            RagNode<Region>* rag_node = ragtemp.find_rag_node(body_rank.id);
            body_rank.size = rag_node->get_size();

            if (body_rank.size <= 100) { //ignore_size) {
                continue;
            }

            bool found_one = false;
            for (typename RagNode<Region>::edge_iterator iter = rag_node->edge_begin(); iter != rag_node->edge_end(); ++iter) {
                if ((*iter)->get_weight() <= max_val && (*iter)->get_weight() >= min_val) {
                    found_one = true;
                    break;
                }
            }
            
            if (found_one) {
                reexamine_bodies.insert(body_rank);
            }
        }

        if (!reexamine_bodies.empty()) {
            body_mode = true;
            head_reexamine = *(reexamine_bodies.begin());
        }

    }
    
    NodePair getTopEdge(Location& location);
    void updatePriority();
    bool isFinished(); 
    void setEdge(NodePair node_pair, double weight);
    unsigned int getNumRemaining() const;
    void removeEdge(NodePair node_pair, bool remove);
    bool undo();
    double getPercentPredictionCorrect()
    {
        if (num_processed) {
            return double(num_correct)/num_processed * 100.0;
        } else {
            return 100.0;
        }
    }
    double getAveragePredictionError()
    {
        if (num_processed) {
            return cum_error/num_processed * 100.0;
        } else {
            return 0.0;
        }
    }
    void export_json(Json::Value& json_writer)
    {
        json_writer["num_processed"] = num_processed;
        json_writer["num_correct"] = num_correct;
        json_writer["cum_error"] = cum_error;
        json_writer["range"][(unsigned int)(0)] = min_val;
        json_writer["range"][(unsigned int)(1)] = max_val;
        json_writer["prune_small_edges"] = prune_small_edges;
        json_writer["body_mode"] = body_mode;
        json_writer["ignore_size"] = ignore_size;
   
        int i = 0; 
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        for (typename std::set<BodyRank>::iterator iter = reexamine_bodies.begin(); iter != reexamine_bodies.end(); ++iter, ++i) {
            RagNode<Region>* rag_node = ragtemp.find_rag_node(iter->id);
            if (!rag_node) {
                --i;
                continue;
            }
            json_writer["reexamine_bodies"][i] = iter->id;   
        }
    }

  private:
    struct BodyRank {
        Region id;
        unsigned long long size;
        bool operator<(const BodyRank& br2) const
        {
            if ((size > br2.size) || (size == br2.size && id < br2.id)) {
                return true;
            }
            return false;
        }
    };

    typename EdgeRanking<Region>::type edge_ranking;
    double min_val, max_val, start_val; 
    
    double curr_prob;
    int curr_decision;

    unsigned int num_processed;    
    unsigned int num_correct;
    double cum_error;
    
    double last_prob;
    int last_decision;

    BodyRank last_reexamine;
    BodyRank head_reexamine;

    bool prune_small_edges;
    bool body_mode;
    unsigned int ignore_size;
    std::set<BodyRank> reexamine_bodies;
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
    if (!reexamine_bodies.empty()) {
        return reexamine_bodies.size();
    }
    return (unsigned int)(edge_ranking.size());
}

template <typename Region> void LocalEdgePriority<Region>::updatePriority()
{
    if (!body_mode) {
        edge_ranking = rag_grab_edge_ranking(EdgePriority<Region>::rag, min_val, max_val, start_val, ignore_size);
    } else if (!reexamine_bodies.empty()){
        edge_ranking = rag_grab_edge_ranking(EdgePriority<Region>::rag, min_val, max_val, start_val, ignore_size, head_reexamine.id);
    }
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
    if (ret) {
        curr_prob = last_prob;
        curr_decision = last_decision;
        --num_processed;
        if ((((curr_prob >= 0.50) && curr_decision == 1) ||
                    (curr_prob <= 0.50) && curr_decision == 0)) {
            --num_correct;
        }
        cum_error -= (std::abs(curr_prob - curr_decision));

        head_reexamine = last_reexamine;
        if (body_mode) {
            reexamine_bodies.insert(head_reexamine);
        }

        updatePriority();
    }
    return ret; 
}

template <typename Region> void LocalEdgePriority<Region>::removeEdge(NodePair node_pair, bool remove)
{
    ++num_processed;
    Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
    bool found_anchor = false;

    if (remove) {
        curr_decision = 0;
        // if the larger body is not the head than that means it has already been processed
        // and is now not an orphan
        if (boost::get<0>(node_pair) != head_reexamine.id) {
            found_anchor = true;
        }
        EdgePriority<Region>::removeEdge(node_pair, remove);

        updatePriority();    
    } else {
        curr_decision = 1;
        setEdge(node_pair, last_prob+1.0);
    }

    if (!reexamine_bodies.empty()) {
        last_reexamine = head_reexamine;
        if (edge_ranking.empty() || found_anchor) {
            do {
                reexamine_bodies.erase(reexamine_bodies.begin());
            } while (!reexamine_bodies.empty() && ragtemp.find_rag_node((head_reexamine = *(reexamine_bodies.begin())).id) == 0);

            updatePriority();
        }
    }

    if ((((curr_prob >= 0.50) && curr_decision == 1) ||
        (curr_prob <= 0.50) && curr_decision == 0)) {
        ++num_correct;
    }
    cum_error += (std::abs(curr_prob - curr_decision));
    last_decision = curr_decision;
    last_prob = curr_prob;

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

    last_prob = curr_prob;
    curr_prob = edge->get_weight();

    if (edge->get_node1()->get_size() >= edge->get_node2()->get_size()) {
        return NodePair(edge->get_node1()->get_node_id(), edge->get_node2()->get_node_id());
    } else {
        return NodePair(edge->get_node2()->get_node_id(), edge->get_node1()->get_node_id());
    }
}


}

#endif
