#include "../Rag/RagUtils.h"

#include "LocalEdgePriority.h"

#include <cmath>
#include <iostream>
#include <map>

using std::vector; using std::set; using std::tr1::unordered_set; using std::tr1::unordered_map;
using std::string;
using std::log;
using std::cerr; using std::endl;
using std::pair;
using std::multimap;
using std::make_pair;

namespace NeuroProof {
struct BodyRank {
    Node_uit id;
    unsigned long long size;
    bool operator<(const BodyRank& br2) const
    {
        if ((size > br2.size) || (size == br2.size && id < br2.id)) {
            return true;
        }
        return false;
    }
};

class BodyRankList {
  public:
    BodyRankList(Rag_uit& rag_) : 
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
        Node_uit head_id = first().id;
        stored_ids.erase(head_id);
        body_list.erase(body_list.begin());
        (rag.find_rag_node(head_id))->set_property(ExaminedStr, true);
    }
    
    void remove(Node_uit id)
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
        history.push_back(vector<HistoryElement>());
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

    Rag_uit& rag;
    set<BodyRank> body_list;
    unordered_map<Node_uit, BodyRank> stored_ids;
    bool checkpoint;
    vector<vector<HistoryElement> > history;
    const string OrphanStr;
    const string ExaminedStr;
    const string SynapseStr;
};

vector<Node_uit> LocalEdgePriority::getQAViolators(unsigned int threshold)
{
    vector<Node_uit> violators;

    for (Rag_uit::nodes_iterator iter = rag.nodes_begin(); iter != rag.nodes_end(); ++iter) {
        unsigned long long synapse_weight = 0;
        try {   
            synapse_weight = (*iter)->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }

        bool is_orphan = false;
        try {   

            is_orphan = (*iter)->get_property<bool>(OrphanStr);
        } catch(...) {
            //
        }

        if (is_orphan && ( (synapse_weight > 0) || ((*iter)->get_size() >= threshold) )) {
            violators.push_back((*iter)->get_node_id());
        }
    } 
    return violators;
}


void LocalEdgePriority::set_synapse_mode(double ignore_size_)
{
    reinitialize_scheduler();

    synapse_mode = true;
    ignore_size_orig = ignore_size_;        
    if (body_list) {
        delete body_list;
    } 
    body_list = new BodyRankList(rag);
    current_depth = 0;

    for (Rag_uit::nodes_iterator iter = rag.nodes_begin(); iter != rag.nodes_end(); ++iter) {
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
void LocalEdgePriority::set_body_mode(double ignore_size_, int depth)
{
    reinitialize_scheduler();

    ignore_size_orig = ignore_size_;        
    if (body_list) {
        delete body_list;
    } 
    body_list = new BodyRankList(rag);

    current_depth = depth; 
    volume_size = rag.get_rag_size();
    ignore_size = voi_change(approx_neurite_size, ignore_size_orig, volume_size);

    for (Rag_uit::nodes_iterator iter = rag.nodes_begin(); iter != rag.nodes_end(); ++iter) {
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
void LocalEdgePriority::set_orphan_mode(double ignore_size_, double threshold, bool synapse_orphan)
{
    reinitialize_scheduler();
    orphan_mode = true;

    ignore_size_orig = ignore_size_;    

    if (body_list) {
        delete body_list;
    } 
    body_list = new BodyRankList(rag);
    current_depth = 0;

    for (Rag_uit::nodes_iterator iter = rag.nodes_begin(); iter != rag.nodes_end(); ++iter) {
        unsigned long long synapse_weight = 0;
        try {   
            synapse_weight = (*iter)->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }

        bool is_orphan = false;
        try {   
            is_orphan = (*iter)->get_property<bool>(OrphanStr);
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

// synapse orphan currently not used
void LocalEdgePriority::set_edge_mode(double lower, double upper, double start)
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

void LocalEdgePriority::estimateWork()
{
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
        Location location;
        boost::tuple<Node_uit, Node_uit> pair = getTopEdge(location);
        //    priority_scheduler.setEdge(pair, 0);
        Node_uit node1 = boost::get<0>(pair);
        Node_uit node2 = boost::get<1>(pair);
        RagEdge_uit* temp_edge = rag.find_rag_edge(node1, node2);
        double weight = temp_edge->get_weight();
        int weightint = int(100 * weight);
        if ((rand() % 100) > (weightint)) {
            removeEdge(pair, true);
        } else {
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


void LocalEdgePriority::reinitialize_scheduler()
{
    orphan_mode = false;
    prob_mode = false;
    synapse_mode = false;

    for (Rag_uit::nodes_iterator iter = rag.nodes_begin();
            iter != rag.nodes_end(); ++iter) {
        (*iter)->rm_property(ExaminedStr);
    }

    volume_size = 0;
    //num_processed = 0;
    clear_history();
}

double LocalEdgePriority::voi_change(double size1, double size2, unsigned long long total_volume)
{
    double part1 = log(size1/total_volume)/log(2.0)*size1/total_volume + 
        log(size2/total_volume)/log(2.0)*size2/total_volume;
    double part2 = log((size1 + size2)/total_volume)/log(2.0)*(size1 + size2)/total_volume;
    double part3 = -1 * (part1 - part2);
    return part3;
}

LocalEdgePriority::LocalEdgePriority(Rag_uit& rag_, double min_val_,
        double max_val_, double start_val_, Json::Value& json_vals) : 
    rag(rag_), min_val(min_val_), max_val(max_val_),
    start_val(start_val_), approx_neurite_size(100), body_list(0),
    OrphanStr("orphan"), ExaminedStr("examined"),
    SynapseStr("synapse_weight")
{
    property_names.push_back(string("location"));
    property_names.push_back(string("edge_size"));

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
        for (Rag_uit::edges_iterator iter = rag.edges_begin();
                iter != rag.edges_end(); ++iter) {
            if (((*iter)->get_property<unsigned int>("edge_size")) <= 1) {
                (*iter)->set_weight(10.0);
            }
        }
    } 

    // determine what has been looked at so far
    Json::Value json_already_analyzed = json_vals["already_analyzed"];
    for (unsigned int i = 0; i < json_already_analyzed.size(); ++i) {
        RagNode_uit* rag_node = rag.find_rag_node(json_already_analyzed[i].asUInt());
        rag_node->set_property(ExaminedStr, true);
    }

    // load synapse info
    unsigned long long num_synapses = 0; 
    BodyRankList body_list_synapse(rag);
    Json::Value json_synapse_weights = json_vals["synapse_bodies"];
    for (unsigned int i = 0; i < json_synapse_weights.size(); ++i) {
        BodyRank body_rank;
        body_rank.id = (json_synapse_weights[i])[(unsigned int)(0)].asUInt();
        body_rank.size = (json_synapse_weights[i])[(unsigned int)(1)].asUInt();
        num_synapses += body_rank.size;
        RagNode_uit* rag_node = rag.find_rag_node(body_rank.id);

        // add synapse weight property for node to synapse weight list
        rag_node->set_property(SynapseStr, body_rank.size);

        bool is_examined = false;
        try { 
            is_examined =  rag_node->get_property<bool>(ExaminedStr);
        } catch(...) {
            //
        }
        if (!is_examined) {
            body_list_synapse.insert(body_rank);
        }
    }

    // load orphan info
    BodyRankList body_list_orphan(rag);
    Json::Value json_orphan = json_vals["orphan_bodies"];
    for (unsigned int i = 0; i < json_orphan.size(); ++i) {
        BodyRank body_rank;
        body_rank.id = json_orphan[i].asUInt();
        RagNode_uit* rag_node = rag.find_rag_node(body_rank.id);
        body_rank.size = rag_node->get_size();

        // add orphan property for node to orphan list
        rag_node->set_property(OrphanStr, true);

        unsigned long long synapse_weight = 0;
        try {   
            synapse_weight = rag_node->get_property<unsigned long long>(SynapseStr);
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
        body_list = new BodyRankList(rag);

        volume_size = rag.get_rag_size();
        ignore_size = voi_change(approx_neurite_size, ignore_size, volume_size);


        for (Rag_uit::nodes_iterator iter = rag.nodes_begin();
                iter != rag.nodes_end(); ++iter) {
            // heuristically ignore bodies below a certain size for computation reasons
            bool is_examined = false;
            try { 
                is_examined = (*iter)->get_property<bool>(ExaminedStr);
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

void LocalEdgePriority::export_json(Json::Value& json_writer)
{
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

    // 4 modes supported currently 
    json_writer["orphan_mode"] = orphan_mode;
    json_writer["synapse_mode"] = synapse_mode;
    json_writer["prob_mode"] = prob_mode;

    int i = 0;
    int orphan_count = 0;
    unsigned int synapse_count = 0;
    int examined_count = 0;

    for (Rag_uit::nodes_iterator iter = rag.nodes_begin();
            iter != rag.nodes_end(); ++iter) {
        bool is_orphan = false;
        bool is_examined = false;
        unsigned long long synapse_weight = 0;

        try {
            is_orphan = (*iter)->get_property<bool>(OrphanStr);
        } catch(...) {
            //
        }

        try {
            synapse_weight = (*iter)->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }

        try {
            is_examined = (*iter)->get_property<bool>(ExaminedStr);
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
unsigned int LocalEdgePriority::getNumRemaining() 
{
    if (isFinished()) {
        return 0;        
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
bool LocalEdgePriority::isFinished()
{
    return (edge_ranking.empty());
}

void LocalEdgePriority::setEdge(NodePair node_pair, double weight)
{
    RagEdge_uit* edge = rag.find_rag_edge(boost::get<0>(node_pair), boost::get<1>(node_pair));

    EdgeRanking::iterator iter;
    for (iter = edge_ranking.begin(); iter != edge_ranking.end(); ++iter) {
        if (iter->second == edge) {
            break;
        }
    }
    if (iter != edge_ranking.end()) {
        edge_ranking.erase(iter);
    }

    EdgeHistory history;
    history.node1 = boost::get<0>(node_pair);
    history.node2 = boost::get<1>(node_pair);
    edge = rag.find_rag_edge(history.node1, history.node2);
    history.weight = edge->get_weight();
    history.preserve_edge = edge->is_preserve();
    history.false_edge = edge->is_false_edge();
    history.remove = false;
    history_queue.push_back(history);
    edge->set_weight(weight);}

    // just grabs top edge or errors -- no real computation
boost::tuple<Node_uit, Node_uit> LocalEdgePriority::getTopEdge(Location& location)
{
    RagEdge_uit* edge;
    try {
        if (edge_ranking.empty()) {
            throw ErrMsg("Priority queue is empty");
        }
        edge = edge_ranking.begin()->second;

        location = edge->get_property<Location>("location");
    } catch(ErrMsg& msg) {
        cerr << msg.str << endl;
        throw ErrMsg("Priority scheduler crashed");
    }

    if (edge->get_node1()->get_size() >= edge->get_node2()->get_size()) {
        return NodePair(edge->get_node1()->get_node_id(), edge->get_node2()->get_node_id());
    } else {
        return NodePair(edge->get_node2()->get_node_id(), edge->get_node1()->get_node_id());
    }
}

double LocalEdgePriority::find_path(RagNode_uit* rag_node_head,
        RagNode_uit* rag_node_dest)
{
    grabAffinityPairs(rag_node_head, 0, 0.01, false); 
    AffinityPair apair(rag_node_head->get_node_id(), rag_node_dest->get_node_id()); 

    AffinityPairsLocal::iterator iter = affinity_pairs.find(apair);
    if (iter == affinity_pairs.end()) {
        return 0.0;
    } else {
        return iter->weight;
    }
}

// assumes that body queue has only legitimate entries
void LocalEdgePriority::updatePriority()
{
    // local edge mode
    if (prob_mode) {
        edge_ranking = rag_grab_edge_ranking(rag, min_val, max_val, start_val, ignore_size);
    } else { // node mode 
        edge_ranking.clear();
        while (edge_ranking.empty() && !body_list->empty()) {
            Node_uit head_id = body_list->first().id; 

            // went to another orphan mode before

            RagNode_uit* head_node = rag.find_rag_node(head_id);

            grabAffinityPairs(head_node, current_depth, 0.01, !orphan_mode);
            double total_information_affinity = 0.0;
            double biggest_change = -1.0;
            RagNode_uit* strongest_affinity_node = 0;

            for (AffinityPairsLocal::iterator iter = affinity_pairs.begin();
                    iter != affinity_pairs.end(); ++iter) {
                Node_uit other_id = iter->region1;
                if (head_id == other_id) {
                    other_id = iter->region2;
                }
                RagNode_uit* other_node = rag.find_rag_node(other_id);

                RagEdge_uit* rag_edge = 0; //ragtemp.find_rag_edge(head_node, other_node);

                double local_information_affinity = -1;
                
                // TODO: separate functions for each -- call base class for common
                if (synapse_mode) { 
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

                    if (synapse_weight1 > 0 && synapse_weight2 > 0) {
                        local_information_affinity = iter->weight *
                            voi_change(synapse_weight1, synapse_weight2, volume_size);
                    }
                } else if (orphan_mode) {
                    bool orphan1 = false;
                    try {
                        orphan1 = head_node->get_property<bool>(OrphanStr);
                    } catch(...) {
                        //
                    }

                    bool orphan2 = false;
                    try {
                        orphan2 = other_node->get_property<bool>(OrphanStr);
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
                        strongest_affinity_node = other_node;
                    } else {
                        strongest_affinity_node = rag.find_rag_node(Node_uit(iter->size));
                    }
                    biggest_change = local_information_affinity;
                }
                total_information_affinity += local_information_affinity;
            }

            if (biggest_change >= ignore_size) {
                edge_ranking.insert(make_pair(0, rag.find_rag_edge(
                                strongest_affinity_node, head_node)));
            }


            if (edge_ranking.empty()) {
                body_list->pop();
            } 
        }
    }
}

bool LocalEdgePriority::undo()
{
    bool ret = undo2();
    if (ret) {
        --num_processed;

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
void LocalEdgePriority::removeEdge(NodePair node_pair, bool remove)
{
    ++num_processed;
    if (num_est_remaining > 0) {
        --num_est_remaining;
    }

    if (prob_mode) {
        ++num_edge_processed;
    } else if (synapse_mode) {
        ++num_syn_processed;
    } else if (orphan_mode) {
        ++num_orphan_processed;
    } else {
        ++num_body_processed;
    }

    if (remove) {
        if (!prob_mode) {
            body_list->start_checkpoint();
            BodyRank head_reexamine = body_list->first();

            RagEdge_uit* rag_edge = rag.find_rag_edge(boost::get<0>(node_pair), boost::get<1>(node_pair));

            RagNode_uit* rag_head_node = rag.find_rag_node(head_reexamine.id);
            RagNode_uit* rag_other_node = rag_edge->get_other_node(rag_head_node);

            bool is_orphan = false;
            bool is_orphan_head = false;
            unsigned long long  synapse_weight = 0;
            unsigned long long synapse_weight_head = 0;
            try {
                synapse_weight_head = rag_head_node->get_property<unsigned long long>(SynapseStr);
            } catch(...) {
                //
            }

            try {
                is_orphan = rag_other_node->get_property<bool>(OrphanStr);
            } catch(...) {
                //
            }
            try {
                is_orphan_head = rag_head_node->get_property<bool>(OrphanStr);
            } catch(...) {
                //
            }

            try {
                synapse_weight = rag_other_node->get_property<unsigned long long>(SynapseStr);
            } catch(...) {
                //
            }

            // remove other body from list
            body_list->remove(rag_other_node->get_node_id());
            body_list->remove(head_reexamine.id);

            removeEdge2(node_pair, true, node_properties);

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
            for (AffinityPairsLocal::iterator iter = affinity_pairs.begin();
                    iter != affinity_pairs.end(); ++iter) {
                Node_uit other_id = iter->region1;
                if (master_item.id == other_id) {
                    other_id = iter->region2;
                }
                RagNode_uit* rag_other_node2 = rag.find_rag_node(other_id);

                body_list->remove(other_id); 
                BodyRank item;
                item.id = other_id;
                item.size = rag_other_node2->get_size();



                if (orphan_mode) {
                    bool is_orphan2 = false;
                    try {   
                        is_orphan2 = rag_other_node2->get_property<bool>(OrphanStr);
                    } catch(...) {
                        //
                    }

                    unsigned long long synapse_weight = 0;
                    try {   
                        synapse_weight = rag_other_node2->get_property<unsigned long long>(SynapseStr);
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
                        item.size = rag_other_node2->get_property<unsigned long long>(SynapseStr);
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
            removeEdge2(node_pair, true, node_properties);
            updatePriority(); 
        }

    } else {
        if (!prob_mode) {
            body_list->start_checkpoint();
            setEdge(node_pair, 1.2);
            updatePriority(); 
            body_list->stop_checkpoint();
        } else {
            setEdge(node_pair, 1.2);
        }
    }
}

void LocalEdgePriority::grabAffinityPairs(RagNode_uit* rag_node_head, int path_restriction, double connection_threshold, bool preserve)
{
    best_node_head.rag_node_curr = rag_node_head;
    best_node_head.rag_edge_curr = 0;
    best_node_head.weight= 1.0;
    best_node_head.path = 0;
    Node_uit node_head = rag_node_head->get_node_id();

    best_node_queue.push(best_node_head);
    AffinityPair affinity_pair_head(node_head, node_head);
    affinity_pair_head.weight = 1.0;
    affinity_pair_head.size = 0;

    affinity_pairs.clear();

    while (!best_node_queue.empty()) {
        BestNode best_node_curr = best_node_queue.top();
        AffinityPair affinity_pair_curr(node_head, best_node_curr.rag_node_curr->get_node_id());

        if (affinity_pairs.find(affinity_pair_curr) == affinity_pairs.end()) { 
            for (RagNode_uit::edge_iterator edge_iter = best_node_curr.rag_node_curr->edge_begin();
                    edge_iter != best_node_curr.rag_node_curr->edge_end(); ++edge_iter) {
                // avoid simple cycles
                if (*edge_iter == best_node_curr.rag_edge_curr) {
                    continue;
                }

                // grab other node 
                RagNode_uit* other_node = (*edge_iter)->get_other_node(best_node_curr.rag_node_curr);

                // avoid duplicates
                AffinityPair temp_pair(node_head, other_node->get_node_id());
                if (affinity_pairs.find(temp_pair) != affinity_pairs.end()) {
                    continue;
                }

                if (path_restriction && (best_node_curr.path == path_restriction)) {
                    continue;
                }

                RagEdge_uit* rag_edge_temp = rag.find_rag_edge(rag_node_head, other_node); 
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

                /* 
                   if (edge_prob < 0.06) {

                   } else if (edge_prob < 0.1) {
                   edge_prob -= 0.05;
                   } else if (edge_prob < 0.15) {
                   edge_prob -= 0.10;
                   } else if (edge_prob < 0.4) {
                   edge_prob -= 0.18;
                   } else if (edge_prob < 0.5) {
                   edge_prob -= 0.05;
                   }*/
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

void LocalEdgePriority::removeEdge2(NodePair node_pair, bool remove,
        vector<string>& node_properties)
{
    assert(remove);

    EdgeHistory history;
    history.node1 = boost::get<0>(node_pair);
    history.node2 = boost::get<1>(node_pair);
    RagEdge_uit* edge = rag.find_rag_edge(history.node1, history.node2);
    history.weight = edge->get_weight();
    history.preserve_edge = edge->is_preserve();
    history.false_edge = edge->is_false_edge();
    history.property_list_curr.push_back(edge->get_property_ptr("location"));
    history.property_list_curr.push_back(edge->get_property_ptr("edge_size")); 

    // node id that is kept
    history.remove = true;
    RagNode_uit* node1 = rag.find_rag_node(history.node1);
    RagNode_uit* node2 = rag.find_rag_node(history.node2);
    history.size1 = node1->get_size();
    history.size2 = node2->get_size();

    for (int i = 0; i < node_properties.size(); ++i) {
        string property_type = node_properties[i];
        try {
            PropertyPtr property = node1->get_property_ptr(property_type);
            history.node_properties1[property_type] = property;
        } catch(...) {
            //
        }
        try {
            PropertyPtr property = node2->get_property_ptr(property_type);
            history.node_properties2[property_type] = property;
        } catch(...) {
            //
        }
    }

    for (RagNode_uit::edge_iterator iter = node1->edge_begin(); iter != node1->edge_end(); ++iter) {
        RagNode_uit* other_node = (*iter)->get_other_node(node1);
        if (other_node != node2) {
            history.node_list1.push_back(other_node->get_node_id());
            history.edge_weight1.push_back((*iter)->get_weight());
            history.preserve_edge1.push_back((*iter)->is_preserve());
            history.false_edge1.push_back((*iter)->is_false_edge());

            vector<boost::shared_ptr<Property> > property_list;
            property_list.push_back((*iter)->get_property_ptr("location"));  
            property_list.push_back((*iter)->get_property_ptr("edge_size"));  
            history.property_list1.push_back(property_list);
        }
    } 

    for (RagNode_uit::edge_iterator iter = node2->edge_begin(); iter != node2->edge_end(); ++iter) {
        RagNode_uit* other_node = (*iter)->get_other_node(node2);
        if (other_node != node1) {
            history.node_list2.push_back(other_node->get_node_id());
            history.edge_weight2.push_back((*iter)->get_weight());
            history.preserve_edge2.push_back((*iter)->is_preserve());
            history.false_edge2.push_back((*iter)->is_false_edge());

            vector<boost::shared_ptr<Property> > property_list;
            property_list.push_back((*iter)->get_property_ptr("location"));  
            property_list.push_back((*iter)->get_property_ptr("edge_size"));  
            history.property_list2.push_back(property_list);
        }
    }

    rag_join_nodes(rag, node1, node2, &join_alg); 
    history_queue.push_back(history);
}

void LocalEdgePriority::clear_history()
{
    history_queue.clear();
}


bool LocalEdgePriority::undo2()
{
    if (history_queue.empty()) {
        return false;
    }
    EdgeHistory& history = history_queue.back();

    if (history.remove) {
        rag.remove_rag_node(rag.find_rag_node(history.node1));

        RagNode_uit* temp_node1 = rag.insert_rag_node(history.node1);
        RagNode_uit* temp_node2 = rag.insert_rag_node(history.node2);
        temp_node1->set_size(history.size1);
        temp_node2->set_size(history.size2);

        for (int i = 0; i < history.node_list1.size(); ++i) {
            RagEdge_uit* temp_edge = rag.insert_rag_edge(temp_node1, rag.find_rag_node(history.node_list1[i]));
            temp_edge->set_weight(history.edge_weight1[i]);
            temp_edge->set_preserve(history.preserve_edge1[i]);
            temp_edge->set_false_edge(history.false_edge1[i]);

            temp_edge->set_property_ptr("location", history.property_list1[i][0]);
            temp_edge->set_property_ptr("edge_size", history.property_list1[i][1]);
        } 
        for (int i = 0; i < history.node_list2.size(); ++i) {
            RagEdge_uit* temp_edge = rag.insert_rag_edge(temp_node2, rag.find_rag_node(history.node_list2[i]));
            temp_edge->set_weight(history.edge_weight2[i]);
            temp_edge->set_preserve(history.preserve_edge2[i]);
            temp_edge->set_false_edge(history.false_edge2[i]);

            temp_edge->set_property_ptr("location", history.property_list2[i][0]);
            temp_edge->set_property_ptr("edge_size", history.property_list2[i][1]);
        }

        RagEdge_uit* temp_edge = rag.insert_rag_edge(temp_node1, temp_node2);
        temp_edge->set_weight(history.weight);
        temp_edge->set_preserve(history.preserve_edge); 
        temp_edge->set_false_edge(history.false_edge); 

        temp_edge->set_property_ptr("location", history.property_list_curr[0]);
        temp_edge->set_property_ptr("edge_size", history.property_list_curr[1]);

        for (NodePropertyMap::iterator iter = history.node_properties1.begin(); 
                iter != history.node_properties1.end();
                ++iter) {
            temp_node1->set_property_ptr(iter->first, iter->second);
        }
        for (NodePropertyMap::iterator iter = history.node_properties2.begin();
                iter != history.node_properties2.end();
                ++iter) {
            temp_node2->set_property_ptr(iter->first, iter->second);
        }
    } else {
        RagNode_uit* temp_node1 = rag.find_rag_node(history.node1);
        RagNode_uit* temp_node2 = rag.find_rag_node(history.node2);

        RagEdge_uit* temp_edge = rag.find_rag_edge(temp_node1, temp_node2);
        temp_edge->set_weight(history.weight);
        temp_edge->set_preserve(history.preserve_edge); 
        temp_edge->set_false_edge(history.false_edge); 
    }

    history_queue.pop_back();
    return true;
}

    
// assume that 0 body will never be added as a screen
LocalEdgePriority::EdgeRanking LocalEdgePriority::rag_grab_edge_ranking(Rag_uit& rag,
        double min_val, double max_val, double start_val,
        double ignore_size, Node_uit screen_id)
{
    EdgeRanking ranking;

    for (Rag_uit::edges_iterator iter = rag.edges_begin(); iter != rag.edges_end(); ++iter) {
        if ((screen_id != 0) && ((*iter)->get_node1()->get_node_id() != screen_id) && ((*iter)->get_node2()->get_node_id() != screen_id)) {
            continue;
        }
        
        double val = (*iter)->get_weight();
        if ((val <= max_val) && (val >= min_val)) {
            if (((*iter)->get_node1()->get_size() > ignore_size) && ((*iter)->get_node2()->get_size() > ignore_size)) {
                ranking.insert(std::make_pair(std::abs(val - start_val), *iter));
            }
        } 
    }

    return ranking;
}
}

