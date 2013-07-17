#include "../Rag/RagUtils.h"

#include "EdgeEditor.h"

#include <iostream>
#include <map>

using std::vector; using std::set; using std::tr1::unordered_set; using std::tr1::unordered_map;
using std::string;
using std::cerr; using std::cout; using std::endl;
using std::pair;

namespace NeuroProof {

vector<Node_uit> EdgeEditor::getQAViolators(unsigned int threshold)
{
    vector<Node_uit> violators;

    for (Rag_uit::nodes_iterator iter = rag.nodes_begin(); iter != rag.nodes_end(); ++iter) {
        unsigned long long synapse_weight = 0;
        try {   
            synapse_weight = (*iter)->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }
        bool is_orphan = !((*iter)->is_boundary());

        if (is_orphan && ( (synapse_weight > 0) || ((*iter)->get_size() >= threshold) )) {
            violators.push_back((*iter)->get_node_id());
        }
    } 
    return violators;
}

void EdgeEditor::set_synapse_mode(double ignore_size_)
{
    reinitialize_scheduler();

    ignore_size = ignore_size_;
    synapse_edge_mode->initialize(ignore_size_);
    edge_mode = synapse_edge_mode;
}

// depth currently not set
void EdgeEditor::set_body_mode(double ignore_size_, int depth)
{
    reinitialize_scheduler();
    
    ignore_size = ignore_size_;
    body_edge_mode->initialize(ignore_size_, depth);
    edge_mode = body_edge_mode;
}

// synapse orphan currently not used
void EdgeEditor::set_orphan_mode(double ignore_size_)
{
    reinitialize_scheduler();
    
    ignore_size = ignore_size_;
    orphan_edge_mode->initialize(ignore_size_);
    edge_mode = orphan_edge_mode;
}

void EdgeEditor::set_custom_mode(EdgeRank* edge_mode_)
{
    reinitialize_scheduler();
    edge_mode = edge_mode_;
}

// synapse orphan currently not used
void EdgeEditor::set_edge_mode(double lower, double upper, double start, double ignore_size_)
{
    reinitialize_scheduler();
    
    ignore_size = ignore_size_;
    prob_edge_mode->initialize(lower, upper, start, ignore_size);
    edge_mode = prob_edge_mode;
}

void EdgeEditor::estimateWork()
{
    int num_edges = 0;
    
    while (!(edge_mode->is_finished())) {
        boost::tuple<Node_uit, Node_uit> pair;
        edge_mode->get_top_edge(pair);
        
        Node_uit node1 = boost::get<0>(pair);
        Node_uit node2 = boost::get<1>(pair);
        RagEdge_uit* temp_edge = rag.find_rag_edge(node1, node2);
        
        double weight = temp_edge->get_weight();
        int weightint = int(100 * weight);
        if ((rand() % 100) > (weightint)) {
            edge_mode->examined_edge(pair, true);
        } else {
            edge_mode->examined_edge(pair, false);
        }
        ++num_edges;
    }

    int num_rolls = num_edges;
    while (num_rolls--) {
        undo();
    }
    num_est_remaining = num_edges;
}


void EdgeEditor::reinitialize_scheduler()
{
    volume_size = 0;
    history_queue.clear();
    num_est_remaining = 0;
}

EdgeEditor::EdgeEditor(Rag_uit& rag_, double min_val_,
        double max_val_, double start_val_, Json::Value& json_vals) : 
    rag(rag_), min_val(min_val_), max_val(max_val_),
    start_val(start_val_), SynapseStr("synapse_weight")
{
    reinitialize_scheduler();

    // Dead Cell -- 140 sections 10nm 14156 pixels 

    // default modes checked for now (could make a special mode loader
    // function in the future) -- default to node edge mode
   
    prob_edge_mode = new ProbEdgeRank(&rag);
    orphan_edge_mode = new OrphanRank(&rag);
    synapse_edge_mode = new SynapseRank(&rag);
    body_edge_mode = new NodeSizeRank(&rag);

    string orphan_str = orphan_edge_mode->get_identifier() + string("_mode");
    string prob_str = prob_edge_mode->get_identifier() + string("_mode");
    string synapse_str = synapse_edge_mode->get_identifier() + string("_mode");

    bool orphan_mode = json_vals.get(orphan_str, false).asBool(); 
    bool prob_mode = json_vals.get(prob_str, false).asBool(); 
    bool synapse_mode = json_vals.get(synapse_str, false).asBool(); 

    // set parameters from JSON (explicitly set options later) 
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

    /* -------- initial datastructures used in each mode ----------- */

    node_properties.push_back(SynapseStr);

    // load synapse info
    Json::Value json_synapse_weights = json_vals["synapse_bodies"];
    for (unsigned int i = 0; i < json_synapse_weights.size(); ++i) {
        Node_uit node_syn = (json_synapse_weights[i])[(unsigned int)(0)].asUInt();
        RagNode_uit* rag_node = rag.find_rag_node(node_syn);
        rag_node->set_property(SynapseStr,
                (unsigned long long)((json_synapse_weights[i])[(unsigned int)(1)].asUInt()));
    }

    // load orphan info:  This is a bit of a hack since orphan information should already
    // be in the rag structure.  Since this interface only assumes the given bodies
    // are orphaned, any body not on this list will be given a nominal boundary weight
    Json::Value json_orphan = json_vals["orphan_bodies"];
    unordered_set<Node_uit> orphan_set;
    for (unsigned int i = 0; i < json_orphan.size(); ++i) {
        RagNode_uit* rag_node = rag.find_rag_node(json_orphan[i].asUInt());
        rag_node->set_boundary_size(0);
        orphan_set.insert(rag_node->get_node_id());
    }
    for (Rag_uit::nodes_iterator iter = rag.nodes_begin();
            iter != rag.nodes_end(); ++iter) {
        if (orphan_set.find((*iter)->get_node_id()) == orphan_set.end()) {
            if ((*iter)->get_boundary_size() == 0) {
                (*iter)->set_boundary_size(1);
            }
        }
    }

    if (synapse_mode) {
        ignore_size = json_vals.get("ignore_size", 0.1).asDouble();
        set_synapse_mode(ignore_size);
    } else if (prob_mode) {
        ignore_size = json_vals.get("ignore_size", 27.0).asDouble();
        set_edge_mode(min_val, max_val, start_val, ignore_size);
    } else if (orphan_mode) {
        ignore_size = json_vals.get("ignore_size", double(BIGBODY10NM)).asDouble();
        set_orphan_mode(ignore_size);
    } else {
        ignore_size = json_vals.get("ignore_size", double(BIGBODY10NM)).asDouble();
        set_body_mode(ignore_size, current_depth);
    }
}

EdgeEditor::~EdgeEditor()
{
    delete prob_edge_mode;
    delete orphan_edge_mode;
    delete synapse_edge_mode;
    delete body_edge_mode;
}

void EdgeEditor::export_json(Json::Value& json_writer)
{
    // stats
    json_writer["num_processed"] = num_processed;
    
    json_writer["num_syn_processed"] = num_syn_processed + 
        synapse_edge_mode->get_num_processed();
    json_writer["num_edge_processed"] = num_edge_processed +
        prob_edge_mode->get_num_processed();
    json_writer["num_body_processed"] = num_body_processed +
        body_edge_mode->get_num_processed();
    json_writer["num_orphan_processed"] = num_orphan_processed +
        orphan_edge_mode->get_num_processed();
    
    json_writer["num_est_remaining"] = num_est_remaining;
    json_writer["num_slices"] = num_slices;
    json_writer["current_depth"] = current_depth;

    // probably irrelevant
    json_writer["range"][(unsigned int)(0)] = min_val;
    json_writer["range"][(unsigned int)(1)] = max_val;

    // applicable, in different senses, to all modes 
    json_writer["ignore_size"] = ignore_size;

    // 4 modes supported currently
    string current_mode = edge_mode->get_identifier() + string("_mode");
    json_writer[current_mode] = true;

    int i = 0;
    int orphan_count = 0;
    unsigned int synapse_count = 0;

    for (Rag_uit::nodes_iterator iter = rag.nodes_begin();
            iter != rag.nodes_end(); ++iter) {
        bool is_orphan = !((*iter)->is_boundary());
        unsigned long long synapse_weight = 0;
        try {
            synapse_weight = (*iter)->get_property<unsigned long long>(SynapseStr);
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
    }
}



// size is determined by edges or nodes in queue
unsigned int EdgeEditor::getNumRemaining() 
{
    if (edge_mode->is_finished()) {
        return 0;
    }
    if (num_est_remaining > 0) {
        return num_est_remaining;
    }
    return edge_mode->get_num_remaining();
}

bool EdgeEditor::isFinished()
{
    return edge_mode->is_finished();
}

void EdgeEditor::setEdge(NodePair node_pair, double weight)
{
    EdgeHistory history;
    history.node1 = boost::get<0>(node_pair);
    history.node2 = boost::get<1>(node_pair);
    RagEdge_uit* edge = rag.find_rag_edge(history.node1, history.node2);
    history.weight = edge->get_weight();
    history.preserve_edge = edge->is_preserve();
    history.false_edge = edge->is_false_edge();
    history.remove = false;
    history_queue.push_back(history);
    edge->set_weight(weight);}

// just grabs top edge or errors -- no real computation
boost::tuple<Node_uit, Node_uit> EdgeEditor::getTopEdge(Location& location)
{
    NodePair node_pair;
    try {
        bool status = edge_mode->get_top_edge(node_pair);
        if (!status) {
            throw ErrMsg("No edge can be found");
        }
        RagEdge_uit* edge = rag.find_rag_edge(boost::get<0>(node_pair),
                boost::get<1>(node_pair));
        location = edge->get_property<Location>("location");
    } catch(ErrMsg& msg) {
        cerr << msg.str << endl;
        throw ErrMsg("Priority scheduler crashed");
    }

    return node_pair;
}





bool EdgeEditor::undo()
{
    bool ret = undo2();
    if (ret) {
        --num_processed;
        ++num_est_remaining;
        edge_mode->undo();
    }
    return ret; 
}

// remove other id (as appropriate) from the body list (don't need to mark as examined)
void EdgeEditor::removeEdge(NodePair node_pair, bool remove)
{
    ++num_processed;
    if (num_est_remaining > 0) {
        --num_est_remaining;
    }

    if (remove) {
        RagNode_uit* node_keep = rag.find_rag_node(boost::get<0>(node_pair)); 
        RagNode_uit* node_remove = rag.find_rag_node(boost::get<1>(node_pair));
       
        unsigned long long  synapse_weight1 = 0;
        unsigned long long  synapse_weight2 = 0;

        try {
            synapse_weight1 = node_keep->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }
        try {
            synapse_weight2 = node_remove->get_property<unsigned long long>(SynapseStr);
        } catch(...) {
            //
        }

        removeEdge2(node_pair, true, node_properties);
        node_keep->set_property(SynapseStr, synapse_weight1+synapse_weight2);

    } else {
        setEdge(node_pair, 1.2);
    }

    edge_mode->examined_edge(node_pair, remove);
}



void EdgeEditor::removeEdge2(NodePair node_pair, bool remove,
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
    history.bound_size1 = node1->get_boundary_size();
    history.bound_size2 = node2->get_boundary_size();

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

bool EdgeEditor::undo2()
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
        temp_node1->set_boundary_size(history.bound_size1);
        temp_node2->set_boundary_size(history.bound_size2);

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


}

