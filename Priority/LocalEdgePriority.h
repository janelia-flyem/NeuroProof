#ifndef LOCALEDGEPRIORITY_H
#define LOCALEDGEPRIORITY_H

#include "EdgePriority.h"
#include "../Algorithms/RagAlgs.h"
#include <iostream>
#include <json/value.h>
#include <set>
#include <queue>
#include <cmath>


// ??!!allow undo when no real mode change 
// ??!!keep body the same?! 
// Don -- rav fix: export rag,  does config labels save?, clear undo queue on change, start session bugs


using std::cout; using std::endl;
namespace NeuroProof {

template <typename Region>
class LocalEdgePriority : public EdgePriority<Region> {
  public:
    typedef boost::tuple<Region, Region> NodePair;
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

    LocalEdgePriority(Rag<Region>& rag_, double min_val_, double max_val_, double start_val_, Json::Value& json_vals); 
    void export_json(Json::Value& json_writer);

    void set_synapse_mode(double ignore_size_)
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        reinitialize_scheduler();

        synapse_mode = true;
        ignore_size_orig = ignore_size_;        
        if (body_list) {
            delete body_list;
        } 
        body_list = new BodyRankList(ragtemp, already_analyzed_list);

        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin(); iter != ragtemp.nodes_end(); ++iter) {
            unsigned long long synapse_weight = 0;
            try {   
                synapse_weight = property_list_retrieve_template_property<Region, unsigned long long>(synapse_weight_list, (*iter));
            } catch(...) {
                continue;
            }
            volume_size += synapse_weight;
            BodyRank body_rank;
            body_rank.id = (*iter)->get_node_id();
            body_rank.size = synapse_weight;
            body_list->insert(body_rank);
        }

        ignore_size = voi_change(ignore_size_orig, ignore_size_orig, volume_size);
        updatePriority();
        estimateWork();
    }

    // depth currently not set
    void set_body_mode(double ignore_size_, int depth)
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        reinitialize_scheduler();

        ignore_size_orig = ignore_size_;        
        if (body_list) {
            delete body_list;
        } 
        body_list = new BodyRankList(ragtemp, already_analyzed_list);
        
        volume_size = get_rag_size(&ragtemp);
        ignore_size = voi_change(approx_neurite_size, ignore_size_orig, volume_size);

        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin(); iter != ragtemp.nodes_end(); ++iter) {
            if ((*iter)->get_size() >= ignore_size_orig) {
                BodyRank body_rank;
                body_rank.id = (*iter)->get_node_id();
                body_rank.size = (*iter)->get_size();
                body_list->insert(body_rank);
            }        
        }

        updatePriority();
        estimateWork();
    }

    // synapse orphan currently not used
    void set_orphan_mode(double ignore_size_, double threshold, bool synapse_orphan)
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        reinitialize_scheduler();
        orphan_mode = true;
        min_val = 1.0 - threshold;
        max_val = threshold;
        start_val = min_val;

        ignore_size = ignore_size_;
        ignore_size_orig = ignore_size_;        
        if (body_list) {
            delete body_list;
        } 
        body_list = new BodyRankList(ragtemp, already_analyzed_list);
        
        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin(); iter != ragtemp.nodes_end(); ++iter) {
            bool is_orphan = false;
            try { 
                is_orphan =  property_list_retrieve_template_property<Region, bool>(orphan_property_list, (*iter));
            } catch(...) {
                //
            }

            if (is_orphan) {
                BodyRank body_rank;
                body_rank.id = (*iter)->get_node_id();
                body_rank.size = (*iter)->get_size();
                body_list->insert(body_rank);
            }        
        }

        updatePriority();
        estimateWork();
    }

    // synapse orphan currently not used
    void set_edge_mode(double lower, double upper, double start)
    {
        reinitialize_scheduler();
        prob_mode = true;
        min_val = lower;
        max_val = upper;
        start_val = start;
        ignore_size = 27;
        ignore_size_orig = ignore_size;

        if (body_list) {
            delete body_list;
        }
        body_list = 0; 
        
        updatePriority();
        estimateWork();
    }

    void estimateWork()
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        num_processed = 0;
        num_est_remaining = 0;
        
        //srand(1); 
        int num_edges = 0;
        // estimate work
        while (!isFinished()) {
            typename EdgePriority<Region>::Location location;
            typename boost::tuple<Region, Region> pair = getTopEdge(location);
            //    priority_scheduler.setEdge(pair, 0);
            Region node1 = boost::get<0>(pair);
            Region node2 = boost::get<1>(pair);
            //cout << node1 << " " << node2 << endl;
            RagEdge<Region>* temp_edge = ragtemp.find_rag_edge(node1, node2);
            double weight = temp_edge->get_weight();
            int weightint = int(100 * weight);
            if ((rand() % 100) > (weightint)) {
                //cout << "remove" << endl;
                removeEdge(pair, true);
            } else {
                //cout << "set" << endl;
                removeEdge(pair, false);
            }
            ++num_edges;
        }
       
        int num_rolls = num_edges;
        while (num_rolls--) {
            undo();
        }
        num_est_remaining = num_edges;
        updatePriority();
    }

    NodePair getTopEdge(Location& location);
    void updatePriority();
    bool isFinished(); 
    void setEdge(NodePair node_pair, double weight);
    unsigned int getNumRemaining() const;
    void removeEdge(NodePair node_pair, bool remove);
    bool undo();

  private:
    struct BodyRank {
        //BodyRank() : top_id(false) {}
        Region id;
        //bool top_id;
        unsigned long long size;
        bool operator<(const BodyRank& br2) const
        {
            //if (top_id || (size > br2.size) || (size == br2.size && id < br2.id)) {
            if ((size > br2.size) || (size == br2.size && id < br2.id)) {
                return true;
            }
            return false;
        }
    };

    class BodyRankList {
      public:
        BodyRankList(Rag<Region>& rag_, boost::shared_ptr<PropertyList<Region> > already_analyzed_list_) : 
            rag(rag_), already_analyzed_list(already_analyzed_list_), checkpoint(false) {}
        void insert(BodyRank item)
        {
            if (checkpoint) {
                history[history.size()-1].push_back(HistoryElement(item, true));
            }
            body_list.insert(item);
            stored_ids[item.id] = item;
            property_list_add_template_property(already_analyzed_list, rag.find_rag_node(item.id), false);
        }
        bool empty()
        {
            return body_list.empty();
        }
        size_t size()
        {
            return body_list.size();
        }
        BodyRank first()
        {
            return *(body_list.begin());
        }
        void pop()
        {
            if (checkpoint) {
                history[history.size()-1].push_back(HistoryElement(first(), false));
            }
            Region head_id = first().id;
            stored_ids.erase(head_id);
            body_list.erase(body_list.begin());
            property_list_add_template_property(already_analyzed_list, rag.find_rag_node(head_id), true);
        }
        void remove(Region id)
        {
            if (stored_ids.find(id) == stored_ids.end()) {
                return;
            }

            if (checkpoint) {
                history[history.size()-1].push_back(HistoryElement(stored_ids[id], false));
            }

            body_list.erase(stored_ids[id]);
            stored_ids.erase(id);
            property_list_add_template_property(already_analyzed_list, rag.find_rag_node(id), true);
        }
        void remove(BodyRank item)
        {
            if (checkpoint) {
                history[history.size()-1].push_back(HistoryElement(stored_ids[item.id], false));
            }

            body_list.erase(item);
            stored_ids.erase(item.id);
            property_list_add_template_property(already_analyzed_list, rag.find_rag_node(item.id), true);
        }
        void start_checkpoint()
        {
            checkpoint = true;
            history.push_back(std::vector<HistoryElement>());
        }
        void stop_checkpoint()
        {
            checkpoint = false;
        }
        void undo_one()
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



      private:

        struct HistoryElement {
            HistoryElement(BodyRank item_, bool inserted_node_) : item(item_), inserted_node(inserted_node_) {}
            BodyRank item;
            bool inserted_node;
        };

        Rag<Region>& rag;
        boost::shared_ptr<PropertyList<Region> > already_analyzed_list;
        std::set<BodyRank> body_list;
        std::tr1::unordered_map<Region, BodyRank> stored_ids;
        bool checkpoint;
        std::vector<std::vector<HistoryElement> > history;
    };

    void grabAffinityPairs(RagNode<Region>* rag_node_head, int path_restriction, double connection_threshold);

    void reinitialize_scheduler()
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        orphan_mode = false;
        prob_mode = false;
        synapse_mode = false;
        already_analyzed_list = NodePropertyList<Region>::create_node_list();
        volume_size = 0;
        num_processed = 0;
        EdgePriority<Region>::clear_history();
    }
    
    double voi_change(double size1, double size2, unsigned long long total_volume)
    {
        double part1 = std::log(size1/total_volume)/std::log(2.0)*size1/total_volume + 
                std::log(size2/total_volume)/std::log(2.0)*size2/total_volume;
        double part2 = std::log((size1 + size2)/total_volume)/std::log(2.0)*(size1 + size2)/total_volume;
        double part3 = -1 * (part1 - part2);
        return part3;
    }

    typename EdgeRanking<Region>::type edge_ranking;
    double min_val, max_val, start_val; 
    
    double curr_prob;

    unsigned int num_processed;    
    unsigned int num_slices;
    unsigned int num_est_remaining;    
    
    double last_prob;
    int last_decision;

    bool prune_small_edges;
    bool orphan_mode;
    bool synapse_mode;
    bool prob_mode;
    double ignore_size;
    double ignore_size_orig;
    BodyRankList * body_list;
    int approx_neurite_size;
    unsigned long long volume_size;

    std::vector<std::string> node_properties;
    boost::shared_ptr<PropertyList<Region> > orphan_property_list;
    boost::shared_ptr<PropertyList<Region> > synapse_weight_list;
    boost::shared_ptr<PropertyList<Region> > already_analyzed_list;

    typedef typename AffinityPair<Region>::Hash AffinityPairsLocal;
    AffinityPairsLocal affinity_pairs;
    struct BestNode {
        RagNode<Region>* rag_node_curr;
        RagEdge<Region>* rag_edge_curr;
        double weight;
        int path;
        Region second_node;
    };
    struct BestNodeCmp {
        bool operator()(const BestNode& q1, const BestNode& q2) const
        {
            return (q1.weight < q2.weight);
        }
    };
    typedef std::priority_queue<BestNode, std::vector<BestNode>, BestNodeCmp> BestNodeQueue;
    BestNode best_node_head;
    BestNodeQueue best_node_queue; 

};

template <typename Region> LocalEdgePriority<Region>::LocalEdgePriority(Rag<Region>& rag_, double min_val_, double max_val_, double start_val_, Json::Value& json_vals) : EdgePriority<Region>(rag_), min_val(min_val_), max_val(max_val_), start_val(start_val_), approx_neurite_size(100), body_list(0)
{
    Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
    reinitialize_scheduler();

    // Dead Cell -- 140 sections 10nm 14156 pixels 

    orphan_mode = json_vals.get("orphan_mode", false).asBool(); 
    prob_mode = json_vals.get("prob_mode", true).asBool(); 
    synapse_mode = json_vals.get("synapse_mode", false).asBool(); 

    // set parameters from JSON (explicitly set options later) 
    prune_small_edges = json_vals.get("prune_small_edges", false).asBool(); 
    // prob mode and orphan mode
    Json::Value json_range = json_vals["range"];
    if (!json_range.empty()) {
        min_val = json_range[(unsigned int)(0)].asDouble();
        max_val = json_range[(unsigned int)(1)].asDouble();
        start_val = min_val;
    }
    num_processed = json_vals.get("num_processed", 0).asUInt(); 
    num_est_remaining = json_vals.get("num_est_remaining", 0).asUInt(); 
    num_slices = json_vals.get("num_slices", 250).asUInt();

    approx_neurite_size *= num_slices;


    if (synapse_mode) {
        ignore_size = json_vals.get("ignore_size", 0.1).asDouble();
    } else if (prob_mode) {
        ignore_size = json_vals.get("ignore_size", 27.0).asDouble();
    } else {
        ignore_size = json_vals.get("ignore_size", 1000.0).asDouble();
    }
    ignore_size_orig = ignore_size;

    /* -------- initial datastructures used in each mode ----------- */

    node_properties.push_back("synapse_weight");
    node_properties.push_back("orphan");

    // prob mode -- cannot transfer to prob_mod
    if (prob_mode && prune_small_edges) {
        for (typename Rag<Region>::edges_iterator iter = ragtemp.edges_begin();
                iter != ragtemp.edges_end(); ++iter) {
            if ((rag_retrieve_property<Region, unsigned int>(&ragtemp, *iter, "edge_size") <= 1)) {
                (*iter)->set_weight(10.0);
            }
        }
    } 

    orphan_property_list = NodePropertyList<Region>::create_node_list(); 
    synapse_weight_list = NodePropertyList<Region>::create_node_list();


    ragtemp.bind_property_list("orphan", orphan_property_list);
    ragtemp.bind_property_list("synapse_weight", synapse_weight_list);

    // determine what has been looked at so far
    Json::Value json_already_analyzed = json_vals["already_analyzed"];
    for (unsigned int i = 0; i < json_already_analyzed.size(); ++i) {
        RagNode<Region>* rag_node = ragtemp.find_rag_node(json_already_analyzed[i].asUInt());
        property_list_add_template_property(already_analyzed_list, rag_node, true);
    }


    // load orphan info
    BodyRankList body_list_orphan(ragtemp, already_analyzed_list);
    Json::Value json_orphan = json_vals["orphan_bodies"];
    for (unsigned int i = 0; i < json_orphan.size(); ++i) {
        BodyRank body_rank;
        body_rank.id = json_orphan[i].asUInt();
        RagNode<Region>* rag_node = ragtemp.find_rag_node(body_rank.id);
        body_rank.size = rag_node->get_size();

        // add orphan property for node to orphan list
        property_list_add_template_property(orphan_property_list, rag_node, true);

        if (body_rank.size >= ignore_size) {
            bool found_one = false;
            for (typename RagNode<Region>::edge_iterator iter = rag_node->edge_begin(); iter != rag_node->edge_end(); ++iter) {
                if ((*iter)->get_weight() <= max_val && (*iter)->get_weight() >= min_val) {
                    found_one = true;
                    break;
                }
            }
            if (found_one) {
                body_list_orphan.insert(body_rank);
            }
        }
    }




    // load synapse info
    unsigned long long num_synapses = 0; 
    BodyRankList body_list_synapse(ragtemp, already_analyzed_list);
    Json::Value json_synapse_weights = json_vals["synapse_bodies"];
    for (unsigned int i = 0; i < json_synapse_weights.size(); ++i) {
        BodyRank body_rank;
        body_rank.id = (json_synapse_weights[i])[(unsigned int)(0)].asUInt();
        body_rank.size = (json_synapse_weights[i])[(unsigned int)(1)].asUInt();
        num_synapses += body_rank.size;
        RagNode<Region>* rag_node = ragtemp.find_rag_node(body_rank.id);

        // add synapse weight property for node to synapse weight list
        property_list_add_template_property(synapse_weight_list, rag_node, body_rank.size);

        bool is_examined = false;
        try { 
            is_examined =  property_list_retrieve_template_property<Region, bool>(already_analyzed_list, rag_node);
        } catch(...) {
            //
        }
        if (!is_examined) {
            body_list_synapse.insert(body_rank);
        }
    }

    if (orphan_mode) {
        body_list = new BodyRankList(body_list_orphan);
    } else if (synapse_mode) {
        body_list = new BodyRankList(body_list_synapse);
        volume_size = num_synapses;
        ignore_size = voi_change(ignore_size, ignore_size, volume_size);
    } else if (!prob_mode) {
        body_list = new BodyRankList(ragtemp, already_analyzed_list);

        volume_size = get_rag_size(&ragtemp);
        ignore_size = voi_change(approx_neurite_size, ignore_size, volume_size);


        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin();
                iter != ragtemp.nodes_end(); ++iter) {
            // heuristically ignore bodies below a certain size for computation reasons
            bool is_examined = false;
            try { 
                is_examined =  property_list_retrieve_template_property<Region, bool>(already_analyzed_list, (*iter));
            } catch(...) {
                //
            }

            if ((!is_examined) && (*iter)->get_size() >= ignore_size_orig) {
                BodyRank body_rank;
                body_rank.id = (*iter)->get_node_id();
                body_rank.size = (*iter)->get_size();
                body_list->insert(body_rank);
            }
        }
    }

    updatePriority();
    
    if (num_est_remaining == 0) {
        estimateWork();
    }
}

template <typename Region> void LocalEdgePriority<Region>::export_json(Json::Value& json_writer)
{
    // stats
    json_writer["num_processed"] = num_processed;
    json_writer["num_est_remaining"] = num_est_remaining;
    json_writer["num_slices"] = num_slices;

    // probably irrelevant
    json_writer["range"][(unsigned int)(0)] = min_val;
    json_writer["range"][(unsigned int)(1)] = max_val;
    json_writer["prune_small_edges"] = prune_small_edges;

    // applicable, in different senses, to all modes 
    json_writer["ignore_size"] = ignore_size_orig;

    // 4 modes supported currently 
    json_writer["orphan_mode"] = orphan_mode;
    json_writer["synapse_mode"] = synapse_mode;
    json_writer["prob_mode"] = prob_mode;

    int i = 0;
    Rag<Region>& ragtemp = (EdgePriority<Region>::rag);

    int orphan_count = 0;
    unsigned int synapse_count = 0;
    int examined_count = 0;

    for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin();
            iter != ragtemp.nodes_end(); ++iter) {
        bool is_orphan = false;
        bool is_examined = false;
        unsigned long long synapse_weight = 0;

        try {
            is_orphan =  property_list_retrieve_template_property<Region, bool>(orphan_property_list, (*iter));
        } catch(...) {
            //
        }

        try {
            synapse_weight = property_list_retrieve_template_property<Region, unsigned long long>(synapse_weight_list, (*iter));
        } catch(...) {
            //
        }

        try {
            is_examined =  property_list_retrieve_template_property<Region, bool>(already_analyzed_list, (*iter));
        } catch(...) {
            //
        }

        if (is_orphan) {
            json_writer["orphan_bodies"][orphan_count] = (*iter)->get_node_id();
            ++orphan_count;
        }

        if (synapse_weight > 0) {
            json_writer["synapse_bodies"][synapse_count][(unsigned int)(0)] = (*iter)->get_node_id();
            json_writer["synapse_bodies"][synapse_count][(unsigned int)(1)] = (unsigned int)(synapse_weight);
            ++synapse_count;
        }

        if (is_examined) {
            json_writer["already_analyzed"][examined_count] = (*iter)->get_node_id();
            ++examined_count;
        }
    }
}



// size is determined by edges or nodes in queue
template <typename Region> unsigned int LocalEdgePriority<Region>::getNumRemaining() const
{
    return num_est_remaining;

    /*if (body_list && !body_list->empty()) {
        return body_list->size();
    }
    return (unsigned int)(edge_ranking.size());*/
}

// edge list should always have next assignment unless volume hasn't been updated yet or is finished
template <typename Region> bool LocalEdgePriority<Region>::isFinished()
{
    return (edge_ranking.empty());
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

// just grabs top edge or errors -- no real computation
template <typename Region> boost::tuple<Region, Region> LocalEdgePriority<Region>::getTopEdge(Location& location)
{
    RagEdge<Region>* edge;
    try {
        if (edge_ranking.empty()) {
            throw ErrMsg("Priority queue is empty");
        }
        edge = edge_ranking.begin()->second;
       
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

// assumes that body queue has only legitimate entries
template <typename Region> void LocalEdgePriority<Region>::updatePriority()
{
    // local edge mode
    if (prob_mode) {
        edge_ranking = rag_grab_edge_ranking(EdgePriority<Region>::rag, min_val, max_val, start_val, ignore_size);
    } else { // node mode 
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        edge_ranking.clear();
        while (edge_ranking.empty() && !body_list->empty()) {
            Region head_id = body_list->first().id; 
            if (!orphan_mode) {
                RagNode<Region>* head_node = ragtemp.find_rag_node(head_id);
                
                grabAffinityPairs(head_node, 0, 0.01);
                double total_information_affinity = 0.0;
                double biggest_change = 0.0;
                RagNode<Region>* strongest_affinity_node = 0;

                for (typename AffinityPairsLocal::iterator iter = affinity_pairs.begin();
                        iter != affinity_pairs.end(); ++iter) {
                    Region other_id = iter->region1;
                    if (head_id == other_id) {
                        other_id = iter->region2;
                    }
                    RagNode<Region>* other_node = ragtemp.find_rag_node(other_id);
                    
                    double local_information_affinity = 0;
                    if (!synapse_mode) { 
                        local_information_affinity = iter->weight * voi_change(head_node->get_size(), other_node->get_size(), volume_size);
                    } else {
                        unsigned long long synapse_weight1 = 0;
                        try {
                            synapse_weight1 = property_list_retrieve_template_property<Region, unsigned long long>(synapse_weight_list, head_node);
                        } catch(...) {
                            //
                        }
                        unsigned long long synapse_weight2 = 0;
                        try {
                            synapse_weight2 = property_list_retrieve_template_property<Region, unsigned long long>(synapse_weight_list, other_node);
                        } catch(...) {
                            //
                        }
                        
                        if (synapse_weight1 > 0 && synapse_weight2 > 0) {
                            local_information_affinity = iter->weight *
                                voi_change(synapse_weight1, synapse_weight2, volume_size);
                        }
                    }

                    if (local_information_affinity >= biggest_change) {
                        strongest_affinity_node = ragtemp.find_rag_node(Region(iter->size));
                        biggest_change = local_information_affinity;
                    }
                    total_information_affinity += local_information_affinity;
                }

                //if (total_information_affinity >= ignore_size) {
                if (biggest_change >= ignore_size) {
                    edge_ranking.insert(std::make_pair(0, ragtemp.find_rag_edge(strongest_affinity_node, head_node)));
                }
            
            } else {
                edge_ranking = rag_grab_edge_ranking(EdgePriority<Region>::rag, min_val, max_val, start_val, ignore_size, head_id);
            }
            if (edge_ranking.empty()) {
                body_list->pop();
            } 
        }
    }
}

template <typename Region> bool LocalEdgePriority<Region>::undo()
{
    bool ret = EdgePriority<Region>::undo();
    if (ret) {
        --num_processed;
        ++num_est_remaining;
        if (body_list) {
            body_list->undo_one();
        }
        updatePriority();
    }
    return ret; 
}

// remove other id (as appropriate) from the body list (don't need to mark as examined)
template <typename Region> void LocalEdgePriority<Region>::removeEdge(NodePair node_pair, bool remove)
{
    ++num_processed;
    --num_est_remaining;

    if (remove) {
        if (!prob_mode) {
            body_list->start_checkpoint();
            BodyRank head_reexamine = body_list->first();
            Rag<Region>& ragtemp = (EdgePriority<Region>::rag);

            RagEdge<Region>* rag_edge = ragtemp.find_rag_edge(boost::get<0>(node_pair), boost::get<1>(node_pair));

            RagNode<Region>* rag_head_node = ragtemp.find_rag_node(head_reexamine.id);
            RagNode<Region>* rag_other_node = rag_edge->get_other_node(rag_head_node);

            bool is_orphan = false;
            bool synapse_weight = 0;
            try {
                is_orphan =  property_list_retrieve_template_property<Region, bool>(orphan_property_list, rag_other_node);
            } catch(...) {
                //
            }
            try {
                synapse_weight = property_list_retrieve_template_property<Region, unsigned long long>(synapse_weight_list, rag_other_node);
            } catch(...) {
                //
            }

            // remove other body from list
            body_list->remove(rag_other_node->get_node_id());
            body_list->remove(head_reexamine.id);
           
            EdgePriority<Region>::removeEdge(node_pair, true, node_properties);

            BodyRank item;
            item.id = head_reexamine.id;
            bool switching = false;
            if (boost::get<0>(node_pair) != head_reexamine.id) {
                switching = true;
                //cout << "switch-aroo" << endl;
                item.id = rag_other_node->get_node_id();
            }
            //item.top_id = true;
            
            if ((orphan_mode && is_orphan) || (!orphan_mode && !synapse_mode)) {
                item.size = rag_head_node->get_size();
                body_list->insert(item);
            } else if (synapse_mode) {
                item.size = head_reexamine.size + synapse_weight;
                body_list->insert(item);
            }
            if (!is_orphan) {
                property_list_add_template_property(orphan_property_list, rag_head_node, false);
            }
            if (synapse_weight > 0) {
                unsigned long long synapse_weight_head = 0;
                try {
                    synapse_weight_head = property_list_retrieve_template_property<Region, unsigned long long>(synapse_weight_list, rag_head_node);
                } catch(...) {
                    //
                }
                if (switching) {
                    property_list_add_template_property(synapse_weight_list, rag_other_node, synapse_weight_head+synapse_weight);
                } else {
                    property_list_add_template_property(synapse_weight_list, rag_head_node, synapse_weight_head+synapse_weight);
                }
            }
            
            if (switching) {
                grabAffinityPairs(rag_other_node, 0, 0.01);
            } else {
                grabAffinityPairs(rag_head_node, 0, 0.01);
            }
            for (typename AffinityPairsLocal::iterator iter = affinity_pairs.begin();
                    iter != affinity_pairs.end(); ++iter) {
                Region other_id = iter->region1;
                if (head_reexamine.id == other_id) {
                    other_id = iter->region2;
                }
                RagNode<Region>* rag_other_node2 = ragtemp.find_rag_node(other_id);

                body_list->remove(other_id); 
                BodyRank item;
                item.id = other_id;
                item.size = rag_other_node2->get_size();

                if (synapse_mode) {
                    item.size = 0;
                    try { 
                        item.size = property_list_retrieve_template_property<Region, unsigned long long>(synapse_weight_list, rag_other_node2);
                    } catch(...) {
                        //
                    }
                }
                body_list->insert(item); 
            }
            updatePriority(); 
            body_list->stop_checkpoint();
        } else {
            EdgePriority<Region>::removeEdge(node_pair, true, node_properties);
            updatePriority(); 
        }

    } else {
        if (!prob_mode) {
            body_list->start_checkpoint();
            setEdge(node_pair, 1.0);
            updatePriority(); 
            body_list->stop_checkpoint();
        } else {
            setEdge(node_pair, 1.0);
        }
    }
}




template<typename Region> void LocalEdgePriority<Region>::grabAffinityPairs(RagNode<Region>* rag_node_head, int path_restriction, double connection_threshold)
{
    best_node_head.rag_node_curr = rag_node_head;
    best_node_head.rag_edge_curr = 0;
    best_node_head.weight= 1.0;
    best_node_head.path = 0;
    Region node_head = rag_node_head->get_node_id();
    
    best_node_queue.push(best_node_head);
    AffinityPair<Region> affinity_pair_head(node_head, node_head);
    affinity_pair_head.weight = 1.0;
    affinity_pair_head.size = 0;

    affinity_pairs.clear();
        
    while (!best_node_queue.empty()) {
        BestNode best_node_curr = best_node_queue.top();
        AffinityPair<Region> affinity_pair_curr(node_head, best_node_curr.rag_node_curr->get_node_id());
  
        if (affinity_pairs.find(affinity_pair_curr) == affinity_pairs.end()) { 
            for (typename RagNode<Region>::edge_iterator edge_iter = best_node_curr.rag_node_curr->edge_begin();
                    edge_iter != best_node_curr.rag_node_curr->edge_end(); ++edge_iter) {
                // avoid simple cycles
                if (*edge_iter == best_node_curr.rag_edge_curr) {
                    continue;
                }

                // grab other node 
                RagNode<Region>* other_node = (*edge_iter)->get_other_node(best_node_curr.rag_node_curr);

                // avoid duplicates
                AffinityPair<Region> temp_pair(node_head, other_node->get_node_id());
                if (affinity_pairs.find(temp_pair) != affinity_pairs.end()) {
                    continue;
                }

                if (path_restriction && (best_node_curr.path == path_restriction)) {
                    continue;
                }

                double edge_prob = 1.0 - (*edge_iter)->get_weight();
                if (edge_prob < 0.000001) {
                    continue;
                }
                edge_prob = best_node_curr.weight * edge_prob;
                if (edge_prob < connection_threshold) {
                    continue;
                }

                /*if (best_node_curr.rag_node_curr != rag_node_head) {
                    cout << "shouldn't happen" << endl;
                }*/

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



}

#endif
