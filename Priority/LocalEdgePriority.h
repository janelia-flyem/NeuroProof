#ifndef LOCALEDGEPRIORITY_H
#define LOCALEDGEPRIORITY_H

#include "EdgePriority.h"
#include "../Rag/RagUtils.h"
#include <iostream>
#include <json/value.h>
#include <set>
#include <queue>
#include <cmath>



#define BIGBODY10NM 25000

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

    LocalEdgePriority(Rag<Region>& rag_, double min_val_, double max_val_, double start_val_, Json::Value& json_vals, bool debug_mode_=false); 
    void export_json(Json::Value& json_writer);

    
    std::vector<Region> getQAViolators(unsigned int threshold)
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);

        std::vector<Region> violators;

        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin(); iter != ragtemp.nodes_end(); ++iter) {
            unsigned long long synapse_weight = 0;
            try {   
                synapse_weight = (*iter)->template get_property<unsigned long long>(SynapseStr);
            } catch(...) {
                //
            }

            bool is_orphan = false;
            try {   

                is_orphan = (*iter)->template get_property<bool>(OrphanStr);
            } catch(...) {
                //
            }

            if (is_orphan && ( (synapse_weight > 0) || ((*iter)->get_size() >= threshold) )) {
                violators.push_back((*iter)->get_node_id());
            }
        } 
        return violators;
    }

    void set_debug_bodies()
    {
        for (int i = 0; i < body_edges.size(); ++i) {
            body_edges[i]->set_preserve(true);
        }
        for (int i = 0; i < synapse_edges.size(); ++i) {
            synapse_edges[i]->set_preserve(false);
        }
        for (int i = 0; i < orphan_edges.size(); ++i) {
            orphan_edges[i]->set_preserve(false);
        }
    }
    void set_debug_synapses()
    {
        for (int i = 0; i < body_edges.size(); ++i) {
            body_edges[i]->set_preserve(false);
        }
        for (int i = 0; i < synapse_edges.size(); ++i) {
            synapse_edges[i]->set_preserve(true);
        }
        for (int i = 0; i < orphan_edges.size(); ++i) {
            orphan_edges[i]->set_preserve(false);
        }
    }
    void set_debug_orphans()
    {
        for (int i = 0; i < body_edges.size(); ++i) {
            body_edges[i]->set_preserve(false);
        }
        for (int i = 0; i < synapse_edges.size(); ++i) {
            synapse_edges[i]->set_preserve(false);
        }
        for (int i = 0; i < orphan_edges.size(); ++i) {
            orphan_edges[i]->set_preserve(true);
        }
    }

    void set_synapse_mode(double ignore_size_)
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        reinitialize_scheduler();

        synapse_mode = true;
        ignore_size_orig = ignore_size_;        
        if (body_list) {
            delete body_list;
        } 
        body_list = new BodyRankList(ragtemp);
        current_depth = 0;

        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin(); iter != ragtemp.nodes_end(); ++iter) {
            unsigned long long synapse_weight = 0;
            try {   
                synapse_weight = (*iter)->template get_property<unsigned long long>(SynapseStr);
            } catch(...) {
                //
            }
            if (synapse_weight == 0) {
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
        //estimateWork();
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
        body_list = new BodyRankList(ragtemp);
       
        current_depth = depth; 
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
        //estimateWork();
    }

    // synapse orphan currently not used
    void set_orphan_mode(double ignore_size_, double threshold, bool synapse_orphan)
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        reinitialize_scheduler();
        orphan_mode = true;
        
        ignore_size_orig = ignore_size_;    

        if (body_list) {
            delete body_list;
        } 
        body_list = new BodyRankList(ragtemp);
        current_depth = 0;

        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin(); iter != ragtemp.nodes_end(); ++iter) {
            unsigned long long synapse_weight = 0;
            try {   
                synapse_weight = (*iter)->template get_property<unsigned long long>(SynapseStr);
            } catch(...) {
                //
            }

            bool is_orphan = false;
            try {   
                is_orphan = (*iter)->template get_property<bool>(OrphanStr);
            } catch(...) {
                //
            }

            if (!is_orphan || (is_orphan && (synapse_weight == 0) && ((*iter)->get_size() < ignore_size_orig))) {
                continue;
            } 
            
            BodyRank body_rank;
            body_rank.id = (*iter)->get_node_id();
            body_rank.size = (*iter)->get_size();
            body_list->insert(body_rank);
        }

        ignore_size = 0;
        updatePriority();
    }

    void set_debug_mode(bool reset=false)
    {
        double ignore_size_body = 0.0; 
        double ignore_size_synapse = 0.0; 
        unsigned long long volume_body = 0;
        unsigned long long volume_synapse = 0;

        if (reset) {
            set_body_mode(25000, 0);
            ignore_size_body = ignore_size;
            volume_body = volume_size;
            set_synapse_mode(0.1);
            ignore_size_synapse = ignore_size;
            volume_synapse = volume_size;
        }

        reinitialize_scheduler();
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        debug_mode = true;  
        edge_ranking.clear();
        debug_queue.clear();
        double count = 0;
        debug_count = 0;

        for (typename Rag<Region>::edges_iterator iter = ragtemp.edges_begin();
                        iter != ragtemp.edges_end(); ++iter, count+=1.0) {
            RagNode<Region>* node1 = (*iter)->get_node1();
            RagNode<Region>* node2 = (*iter)->get_node2();
            if (!reset) {
                debug_queue.push_back(OrderedPair<Region>(node1->get_node_id(), node2->get_node_id()));
            } else {
                if ((*iter)->is_preserve() || (*iter)->is_false_edge()) {
                    (*iter)->set_preserve(false);
                    continue;
                }
                bool examined = false;

                // is body mode
                double local_info = voi_change(node1->get_size(), node2->get_size(), volume_body);
                if ((local_info >= ignore_size_body) && (node1->get_size() >= 1000 && node2->get_size() >= 1000)) {
                    examined = true;
                    body_edges.push_back(*iter);
                }

                // is synapse edge
                unsigned long long synapse_weight1 = 0;
                unsigned long long synapse_weight2 = 0;
                if (!examined) {
                    try {
                        synapse_weight1 = node1->template get_property<unsigned long long>(SynapseStr);
                    } catch(...) {
                        //
                    }
                    try {
                        synapse_weight2 = node2->template get_property<unsigned long long>(SynapseStr);
                    } catch(...) {
                        //
                    }
                    
                    if ((synapse_weight1 > 0) && (synapse_weight2 > 0)) {
                        double local_info = voi_change(synapse_weight1, synapse_weight2, volume_synapse);
                        if (local_info >= ignore_size_synapse) {
                            examined = true;
                            synapse_edges.push_back(*iter);
                        }
                    }
                }

                // is orphan edge    
                if (!examined && ((*iter)->get_weight() <= 0.99)) {
                    bool orphan1 = false;
         
                    try {
                        orphan1 = node1->template get_property<bool>(OrphanStr);
                    } catch(...) {
                        //
                    }

                    bool orphan2 = false;
                    try {
                        orphan2 = node2->template get_property<bool>(OrphanStr);
                    } catch(...) {
                        //
                    }
                
                    if (orphan1 && !orphan2 && ((synapse_weight1 > 0) || (node1->get_size() >= 25000))) {
                        examined = true;
                        orphan_edges.push_back(*iter);
                    } else if (orphan2 && !orphan1 && ((synapse_weight2 > 0) || (node2->get_size() >= 25000))) {
                        examined = true;
                        orphan_edges.push_back(*iter);
                    }
                }

                /*if (examined) {
                    edge_ranking.insert(std::make_pair(count, ragtemp.find_rag_edge(strongest_affinity_node, head_node)));
                }*/

                (*iter)->set_weight(0.5);
                (*iter)->set_preserve(examined);
            }
        }
    }


    // synapse orphan currently not used
    void set_edge_mode(double lower, double upper, double start)
    {
        reinitialize_scheduler();
        prob_mode = true;
        min_val = lower;
        max_val = upper;
        start_val = start;
        ignore_size = 1;
        ignore_size_orig = ignore_size;
        num_est_remaining = 0;

        if (body_list) {
            delete body_list;
        }
        body_list = 0; 
        
        updatePriority();
        //estimateWork();
    }

    void estimateWork()
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        
        unsigned int num_processed_old = num_processed; 
        unsigned int num_syn_processed_old = num_syn_processed; 
        unsigned int num_body_processed_old = num_body_processed; 
        unsigned int num_orphan_processed_old = num_orphan_processed; 
        unsigned int num_edge_processed_old = num_edge_processed; 
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

        num_processed = num_processed_old;
        num_syn_processed = num_syn_processed_old;
        num_body_processed = num_body_processed_old;
        num_orphan_processed = num_orphan_processed_old;
        num_edge_processed = num_edge_processed_old;
    }

    NodePair getTopEdge(Location& location);
    void updatePriority();
    bool isFinished(); 
    void setEdge(NodePair node_pair, double weight);
    unsigned int getNumRemaining();
    void removeEdge(NodePair node_pair, bool remove);
    bool undo();
    double find_path(RagNode<Region>* rag_node_head, RagNode<Region>* rag_node_dest);

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
        BodyRankList(Rag<Region>& rag_) : 
            rag(rag_), checkpoint(false), OrphanStr("orphan"), ExaminedStr("examined"),
            SynapseStr("synapse_weight") {}
        void insert(BodyRank item)
        {
            if (checkpoint) {
                history[history.size()-1].push_back(HistoryElement(item, true));
            }
            body_list.insert(item);
            stored_ids[item.id] = item;
            (rag.find_rag_node(item.id))->set_property(ExaminedStr, false);
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
            (rag.find_rag_node(head_id))->set_property(ExaminedStr, true);
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
            (rag.find_rag_node(id))->set_property(ExaminedStr, true);
        }
        void remove(BodyRank item)
        {
            if (checkpoint) {
                history[history.size()-1].push_back(HistoryElement(stored_ids[item.id], false));
            }

            body_list.erase(item);
            stored_ids.erase(item.id);
            (rag.find_rag_node(item.id))->set_property(ExaminedStr, true);
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
        std::set<BodyRank> body_list;
        std::tr1::unordered_map<Region, BodyRank> stored_ids;
        bool checkpoint;
        std::vector<std::vector<HistoryElement> > history;
        const std::string OrphanStr;
        const std::string ExaminedStr;
        const std::string SynapseStr;
    };

    void grabAffinityPairs(RagNode<Region>* rag_node_head, int path_restriction, double connection_threshold, bool preserve);

    void reinitialize_scheduler()
    {
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        orphan_mode = false;
        prob_mode = false;
        synapse_mode = false;
        
        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin();
                iter != ragtemp.nodes_end(); ++iter) {
            (*iter)->rm_property(ExaminedStr);
        }
        
        volume_size = 0;
        //num_processed = 0;
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
    unsigned int num_syn_processed;    
    unsigned int num_body_processed;    
    unsigned int num_edge_processed;    
    unsigned int num_orphan_processed;    
    unsigned int num_slices;
    unsigned int current_depth;
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
    bool debug_mode;
    unsigned int debug_count;

    Json::Value json_body_edges;
    Json::Value json_synapse_edges;
    Json::Value json_orphan_edges;

    std::vector<OrderedPair<Region> > debug_queue;
    std::vector<RagEdge<Region>* > body_edges;
    std::vector<RagEdge<Region>* > synapse_edges;
    std::vector<RagEdge<Region>* > orphan_edges;

    const std::string OrphanStr;
    const std::string ExaminedStr;
    const std::string SynapseStr;
};

template <typename Region> LocalEdgePriority<Region>::LocalEdgePriority(Rag<Region>& rag_, double min_val_, double max_val_, double start_val_, Json::Value& json_vals, bool debug_mode_) : EdgePriority<Region>(rag_), min_val(min_val_), max_val(max_val_), start_val(start_val_), approx_neurite_size(100), body_list(0), debug_mode(debug_mode_), OrphanStr("orphan"), ExaminedStr("examined"), SynapseStr("synapse_weight")
{
    Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
    reinitialize_scheduler();

    // Dead Cell -- 140 sections 10nm 14156 pixels 

    orphan_mode = json_vals.get("orphan_mode", false).asBool(); 
    prob_mode = json_vals.get("prob_mode", false).asBool(); 
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
    num_syn_processed = json_vals.get("num_syn_processed", 0).asUInt(); 
    num_edge_processed = json_vals.get("num_edge_processed", 0).asUInt(); 
    num_body_processed = json_vals.get("num_body_processed", 0).asUInt(); 
    num_orphan_processed = json_vals.get("num_orphan_processed", 0).asUInt(); 
    num_est_remaining = json_vals.get("num_est_remaining", 0).asUInt(); 
    num_slices = json_vals.get("num_slices", 250).asUInt();
    current_depth = json_vals.get("current_depth", 0).asUInt();

    approx_neurite_size *= num_slices;

    json_body_edges = json_vals["body_edges"];
    json_synapse_edges = json_vals["synapse_edges"];
    json_orphan_edges = json_vals["orphan_edges"];

    if (synapse_mode) {
        ignore_size = json_vals.get("ignore_size", 0.1).asDouble();
    } else if (prob_mode) {
        ignore_size = json_vals.get("ignore_size", 27.0).asDouble();
    } else if (orphan_mode) {
        ignore_size = json_vals.get("ignore_size", double(BIGBODY10NM)).asDouble();
    } else {
        ignore_size = json_vals.get("ignore_size", double(BIGBODY10NM)).asDouble();
    }
    ignore_size_orig = ignore_size;

    /* -------- initial datastructures used in each mode ----------- */

    node_properties.push_back(SynapseStr);
    node_properties.push_back(OrphanStr);

    // prob mode -- cannot transfer to prob_mod
    if (prob_mode && prune_small_edges) {
        for (typename Rag<Region>::edges_iterator iter = ragtemp.edges_begin();
                iter != ragtemp.edges_end(); ++iter) {
            if (((*iter)->template get_property<unsigned int>("edge_size")) <= 1) {
                (*iter)->set_weight(10.0);
            }
        }
    } 

    // determine what has been looked at so far
    Json::Value json_already_analyzed = json_vals["already_analyzed"];
    for (unsigned int i = 0; i < json_already_analyzed.size(); ++i) {
        RagNode<Region>* rag_node = ragtemp.find_rag_node(json_already_analyzed[i].asUInt());
        rag_node->set_property(ExaminedStr, true);
    }







    // load synapse info
    unsigned long long num_synapses = 0; 
    BodyRankList body_list_synapse(ragtemp);
    Json::Value json_synapse_weights = json_vals["synapse_bodies"];
    for (unsigned int i = 0; i < json_synapse_weights.size(); ++i) {
        BodyRank body_rank;
        body_rank.id = (json_synapse_weights[i])[(unsigned int)(0)].asUInt();
        body_rank.size = (json_synapse_weights[i])[(unsigned int)(1)].asUInt();
        num_synapses += body_rank.size;
        RagNode<Region>* rag_node = ragtemp.find_rag_node(body_rank.id);

        // add synapse weight property for node to synapse weight list
        rag_node->set_property(SynapseStr, body_rank.size);

        bool is_examined = false;
        try { 
            is_examined =  rag_node->template get_property<bool>(ExaminedStr);
        } catch(...) {
            //
        }
        if (!is_examined) {
            body_list_synapse.insert(body_rank);
        }
    }


    // load orphan info
    BodyRankList body_list_orphan(ragtemp);
    Json::Value json_orphan = json_vals["orphan_bodies"];
    for (unsigned int i = 0; i < json_orphan.size(); ++i) {
        BodyRank body_rank;
        body_rank.id = json_orphan[i].asUInt();
        RagNode<Region>* rag_node = ragtemp.find_rag_node(body_rank.id);
        body_rank.size = rag_node->get_size();

        // add orphan property for node to orphan list
        rag_node->set_property(OrphanStr, true);

        unsigned long long synapse_weight = 0;
        try {   
            synapse_weight = rag_node->template get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }

        if (body_rank.size >= ignore_size_orig || (synapse_weight > 0)) {
            body_list_orphan.insert(body_rank);
        }
    }



    if (orphan_mode) {
        body_list = new BodyRankList(body_list_orphan);
        volume_size = 0;
        ignore_size = 0;
    } else if (synapse_mode) {
        body_list = new BodyRankList(body_list_synapse);
        volume_size = num_synapses;
        ignore_size = voi_change(ignore_size, ignore_size, volume_size);
    } else if (!prob_mode) {
        body_list = new BodyRankList(ragtemp);

        volume_size = get_rag_size(&ragtemp);
        ignore_size = voi_change(approx_neurite_size, ignore_size, volume_size);


        for (typename Rag<Region>::nodes_iterator iter = ragtemp.nodes_begin();
                iter != ragtemp.nodes_end(); ++iter) {
            // heuristically ignore bodies below a certain size for computation reasons
            bool is_examined = false;
            try { 
                is_examined = (*iter)->template get_property<bool>(ExaminedStr);
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
        //estimateWork();
    }
}

template <typename Region> void LocalEdgePriority<Region>::export_json(Json::Value& json_writer)
{
    if (debug_mode) {
        if (body_edges.empty() && synapse_edges.empty() && orphan_edges.empty()) {
            if (!json_body_edges.isNull()) {
                json_writer["body_edges"] = json_body_edges; 
            }
            if (!json_synapse_edges.isNull()) {
                json_writer["synapse_edges"] = json_synapse_edges; 
            }
            if (!json_orphan_edges.isNull()) {
                json_writer["orphan_edges"] = json_orphan_edges; 
            }
        } else {
            for (int i = 0; i < body_edges.size(); ++i) {
                Json::Value edge;
                edge["node1"] = body_edges[i]->get_node1()->get_node_id();
                edge["node2"] = body_edges[i]->get_node2()->get_node_id();
                json_writer["body_edges"][i] = edge; 
            }
            for (int i = 0; i < synapse_edges.size(); ++i) {
                Json::Value edge;
                edge["node1"] = synapse_edges[i]->get_node1()->get_node_id();
                edge["node2"] = synapse_edges[i]->get_node2()->get_node_id();
                json_writer["synapse_edges"][i] = edge; 
            }
            for (int i = 0; i < orphan_edges.size(); ++i) {
                Json::Value edge;
                edge["node1"] = orphan_edges[i]->get_node1()->get_node_id();
                edge["node2"] = orphan_edges[i]->get_node2()->get_node_id();
                json_writer["orphan_edges"][i] = edge; 
            }
        }
    }
    
    // stats
    json_writer["num_processed"] = num_processed;
    json_writer["num_syn_processed"] = num_syn_processed;
    json_writer["num_edge_processed"] = num_edge_processed;
    json_writer["num_body_processed"] = num_body_processed;
    json_writer["num_orphan_processed"] = num_orphan_processed;
    json_writer["num_est_remaining"] = num_est_remaining;
    json_writer["num_slices"] = num_slices;
    json_writer["current_depth"] = current_depth;

    // probably irrelevant
    json_writer["range"][(unsigned int)(0)] = min_val;
    json_writer["range"][(unsigned int)(1)] = max_val;
    json_writer["prune_small_edges"] = prune_small_edges;

    // applicable, in different senses, to all modes 
    json_writer["ignore_size"] = ignore_size_orig;

    if (debug_mode) {
        json_writer["orphan_mode"] = false;
        json_writer["synapse_mode"] = false; 
        json_writer["prob_mode"] = false;
        return;
    }

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
            is_orphan = (*iter)->template get_property<bool>(OrphanStr);
        } catch(...) {
            //
        }

        try {
            synapse_weight = (*iter)->template get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }

        try {
            is_examined = (*iter)->template get_property<bool>(ExaminedStr);
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
template <typename Region> unsigned int LocalEdgePriority<Region>::getNumRemaining() 
{
    if (isFinished()) {
        return 0;        
    }
    
    if (debug_mode) {
        return (unsigned int)(debug_queue.size() - debug_count);
    }

    if (num_est_remaining > 0) {
        return num_est_remaining;
    }

    if (body_list && !body_list->empty()) {
        return body_list->size();
    }
    return (unsigned int)(edge_ranking.size());
}

// edge list should always have next assignment unless volume hasn't been updated yet or is finished
template <typename Region> bool LocalEdgePriority<Region>::isFinished()
{
    if (debug_mode) {
        if ((debug_queue.size() - debug_count)) {
            return false;
        }
        return true;
    }
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
        Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
        if (debug_mode) {
            if (debug_queue.size() == debug_count) {
                throw ErrMsg("Debug queue is empty");
            }
            edge = ragtemp.find_rag_edge(debug_queue[debug_count].region1, debug_queue[debug_count].region2);
            if (!edge) {
                throw ErrMsg("Edge not found in debug queue");
            }
        } else {
            if (edge_ranking.empty()) {
                throw ErrMsg("Priority queue is empty");
            }
            edge = edge_ranking.begin()->second;
        }
       
        location = edge->template get_property<Location>("location");
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

template <typename Region> double LocalEdgePriority<Region>::find_path(RagNode<Region>* rag_node_head,
        RagNode<Region>* rag_node_dest)
{
    grabAffinityPairs(rag_node_head, 0, 0.01, false); 
    AffinityPair<Region> apair(rag_node_head->get_node_id(), rag_node_dest->get_node_id()); 
    
    typename AffinityPairsLocal::iterator iter = affinity_pairs.find(apair);
    if (iter == affinity_pairs.end()) {
        return 0.0;
    } else {
        return iter->weight;
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
            
            // went to another orphan mode before

            RagNode<Region>* head_node = ragtemp.find_rag_node(head_id);

            grabAffinityPairs(head_node, current_depth, 0.01, !orphan_mode);
            double total_information_affinity = 0.0;
            double biggest_change = -1.0;
            RagNode<Region>* strongest_affinity_node = 0;

            for (typename AffinityPairsLocal::iterator iter = affinity_pairs.begin();
                    iter != affinity_pairs.end(); ++iter) {
                Region other_id = iter->region1;
                if (head_id == other_id) {
                    other_id = iter->region2;
                }
                RagNode<Region>* other_node = ragtemp.find_rag_node(other_id);

                RagEdge<Region>* rag_edge = 0; //ragtemp.find_rag_edge(head_node, other_node);
                
                double local_information_affinity = -1;
                if (synapse_mode) { 
                    unsigned long long synapse_weight1 = 0;
                    try {
                        synapse_weight1 = head_node->template get_property<unsigned long long>(SynapseStr);
                    } catch(...) {
                        //
                    }
                    unsigned long long synapse_weight2 = 0;
                    try {
                        synapse_weight2 = other_node->template get_property<unsigned long long>(SynapseStr);
                    } catch(...) {
                        //
                    }

                    if (synapse_weight1 > 0 && synapse_weight2 > 0) {
                        local_information_affinity = iter->weight *
                            voi_change(synapse_weight1, synapse_weight2, volume_size);
                    }
                } else if (orphan_mode) {
                    bool orphan1 = false;
                    try {
                        orphan1 = head_node->template get_property<bool>(OrphanStr);
                    } catch(...) {
                        //
                    }

                    bool orphan2 = false;
                    try {
                        orphan2 = other_node->template get_property<bool>(OrphanStr);
                    } catch(...) {
                        //
                    }

                    if (orphan1 && !orphan2) {
                        local_information_affinity = iter->weight; //other_node->get_size() * iter->weight;
                    }
                } else {
                   
                    local_information_affinity = iter->weight * voi_change(head_node->get_size(), other_node->get_size(), volume_size);
                    if (other_node->get_size() < 1000) {
                        local_information_affinity = 0.0;
                    }
                }

                if (local_information_affinity >= biggest_change) {
                    if (rag_edge) {
                        //cout << "using local edge instead" << endl;
                        strongest_affinity_node = other_node;
                    } else {
                        strongest_affinity_node = ragtemp.find_rag_node(Region(iter->size));
                    }
                    biggest_change = local_information_affinity;
                }
                total_information_affinity += local_information_affinity;
            }

            //if (total_information_affinity >= ignore_size) {
            if (biggest_change >= ignore_size) {
                edge_ranking.insert(std::make_pair(0, ragtemp.find_rag_edge(strongest_affinity_node, head_node)));
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
    
        if (debug_mode) {
            assert(debug_count > 0);
            --debug_count;
            return true;
        }   
    
        if (prob_mode) {
            --num_edge_processed;
        } else if (synapse_mode) {
            --num_syn_processed;
        } else if (orphan_mode) {
            --num_orphan_processed;
        } else {
            --num_body_processed;
        }

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
    if (num_est_remaining > 0) {
        --num_est_remaining;
    }

    if (prob_mode) {
        ++num_edge_processed;
    } else if (debug_mode) {
        //   
    } else if (synapse_mode) {
        ++num_syn_processed;
    } else if (orphan_mode) {
        ++num_orphan_processed;
    } else {
        ++num_body_processed;
    }

    if (remove) {
        if (debug_mode) {
            setEdge(node_pair, 0.0);
            ++debug_count;
        } else if (!prob_mode) {
            body_list->start_checkpoint();
            BodyRank head_reexamine = body_list->first();
            Rag<Region>& ragtemp = (EdgePriority<Region>::rag);

            RagEdge<Region>* rag_edge = ragtemp.find_rag_edge(boost::get<0>(node_pair), boost::get<1>(node_pair));

            RagNode<Region>* rag_head_node = ragtemp.find_rag_node(head_reexamine.id);
            RagNode<Region>* rag_other_node = rag_edge->get_other_node(rag_head_node);

            bool is_orphan = false;
            bool is_orphan_head = false;
            unsigned long long  synapse_weight = 0;
            unsigned long long synapse_weight_head = 0;
            try {
                synapse_weight_head = rag_head_node->template get_property<unsigned long long>(SynapseStr);
            } catch(...) {
                //
            }

            try {
                is_orphan = rag_other_node->template get_property<bool>(OrphanStr);
            } catch(...) {
                //
            }
            try {
                is_orphan_head = rag_head_node->template get_property<bool>(OrphanStr);
            } catch(...) {
                //
            }

            try {
                synapse_weight = rag_other_node->template get_property<unsigned long long>(SynapseStr);
            } catch(...) {
                //
            }

            // remove other body from list
            body_list->remove(rag_other_node->get_node_id());
            body_list->remove(head_reexamine.id);
        

            EdgePriority<Region>::removeEdge(node_pair, true, node_properties);

            BodyRank master_item;
            master_item.id = head_reexamine.id;
            master_item.size = head_reexamine.size;
            bool switching = false;
            if (boost::get<0>(node_pair) != head_reexamine.id) {
                //assert(orphan_mode || synapse_mode);
                switching = true;
                //cout << "switch-aroo" << endl;
                master_item.id = rag_other_node->get_node_id();
                master_item.size = rag_other_node->get_size();
            }
            //item.top_id = true;
            
            if ((orphan_mode && (is_orphan && is_orphan_head)) || (!orphan_mode && !synapse_mode)) {
                body_list->insert(master_item);
            } else if (synapse_mode) {
                master_item.size = head_reexamine.size + synapse_weight;
                body_list->insert(master_item);
            }
            if ((!is_orphan) || (!is_orphan_head)) {
                if (switching) {
                    rag_other_node->set_property(OrphanStr, false);
                } else {
                    rag_head_node->set_property(OrphanStr, false);
                }
            }
            
            if ((synapse_weight + synapse_weight_head) > 0) {

                if (switching) {
                    rag_other_node->set_property(SynapseStr, synapse_weight_head+synapse_weight);
                } else {
                    rag_head_node->set_property(SynapseStr, synapse_weight_head+synapse_weight);
                }
            }
            
            if (switching) {
                grabAffinityPairs(rag_other_node, current_depth, 0.01, !orphan_mode);
            } else {
                grabAffinityPairs(rag_head_node, current_depth, 0.01, !orphan_mode);
            }
            for (typename AffinityPairsLocal::iterator iter = affinity_pairs.begin();
                    iter != affinity_pairs.end(); ++iter) {
                Region other_id = iter->region1;
                if (master_item.id == other_id) {
                    other_id = iter->region2;
                }
                RagNode<Region>* rag_other_node2 = ragtemp.find_rag_node(other_id);

                body_list->remove(other_id); 
                BodyRank item;
                item.id = other_id;
                item.size = rag_other_node2->get_size();



                if (orphan_mode) {
                    bool is_orphan2 = false;
                    try {   
                        is_orphan2 = rag_other_node2->template get_property<bool>(OrphanStr);
                    } catch(...) {
                        //
                    }

                    unsigned long long synapse_weight = 0;
                    try {   
                        synapse_weight = rag_other_node2->template get_property<unsigned long long>(SynapseStr);
                    } catch(...) {
                        //
                    }
                    
                    if (!is_orphan2 || ((item.size < ignore_size_orig) && synapse_weight == 0)) {
                        continue;
                    }  
                }


                if (synapse_mode) {
                    item.size = 0;
                    try { 
                        item.size = rag_other_node2->template get_property<unsigned long long>(SynapseStr);
                    } catch(...) {
                        //
                    }

                    if (item.size == 0) {
                        continue;
                    }
                }

                if (!orphan_mode && !synapse_mode) {
                    if (item.size < ignore_size_orig) {
                        continue;
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
        if (debug_mode) {
            setEdge(node_pair, 1.0);
            ++debug_count;
        } else if (!prob_mode) {
            body_list->start_checkpoint();
            setEdge(node_pair, 1.2);
            updatePriority(); 
            body_list->stop_checkpoint();
        } else {
            setEdge(node_pair, 1.2);
        }
    }
}




template<typename Region> void LocalEdgePriority<Region>::grabAffinityPairs(RagNode<Region>* rag_node_head, int path_restriction, double connection_threshold, bool preserve)
{
    Rag<Region>& ragtemp = (EdgePriority<Region>::rag);
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

                RagEdge<Region>* rag_edge_temp = ragtemp.find_rag_edge(rag_node_head, other_node); 
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



}

#endif
