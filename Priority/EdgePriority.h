#ifndef EDGEPRIORITY_H
#define EDGEPRIORITY_H

#include "../DataStructures/Rag.h"
#include <vector>
#include <boost/tuple/tuple.hpp>

namespace NeuroProof {

template <typename Region>
class EdgePriority {
  public:
    typedef boost::tuple<Region, Region> NodePair;
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

    EdgePriority(Rag<Region>& rag_) : rag(rag_), updated(false)
    {
        property_names.push_back(std::string("location"));
        property_names.push_back(std::string("edge_size"));
    }
    virtual NodePair getTopEdge(Location& location) = 0;
    virtual bool isFinished() = 0;
    virtual void updatePriority() = 0;
    // low weight == no connection
    virtual void setEdge(NodePair node_pair, double weight);
    virtual bool undo(RagEdge<Region>** edge = 0);
    virtual void removeEdge(NodePair node_pair, bool remove) = 0;
    void clear_history();

  protected:
    virtual void removeEdge(NodePair node_pair, bool remove, std::vector<std::string>& node_properties);
    void setUpdated(bool status);
    bool isUpdated(); 
    Rag<Region>& rag;
    
  private:
    typedef std::tr1::unordered_map<std::string, boost::shared_ptr<Property> > NodePropertyMap; 
    struct EdgeHistory {
        Region node1;
        Region node2;
        unsigned long long size1;
        unsigned long long size2;
        std::vector<Region> node_list1;
        std::vector<Region> node_list2;
        NodePropertyMap node_properties1;
        NodePropertyMap node_properties2;
        std::vector<double> edge_weight1;
        std::vector<double> edge_weight2;
        std::vector<bool> preserve_edge1;
        std::vector<bool> preserve_edge2;
        std::vector<bool> false_edge1;
        std::vector<bool> false_edge2;
        std::vector<std::vector<boost::shared_ptr<Property> > > property_list1;
        std::vector<std::vector<boost::shared_ptr<Property> > > property_list2;
        bool remove;
        double weight;
        bool false_edge;
        bool preserve_edge;
        std::vector<boost::shared_ptr<Property> > property_list_curr;
    };

    std::vector<EdgeHistory> history_queue;
    std::vector<std::string> property_names;
    bool updated;
};


template <typename Region> void EdgePriority<Region>::setUpdated(bool status)
{
    updated = status;
}

template <typename Region> bool EdgePriority<Region>::isUpdated()
{
    return updated;
}

template <typename Region> void EdgePriority<Region>::setEdge(NodePair node_pair, double weight)
{
    EdgeHistory history;
    history.node1 = boost::get<0>(node_pair);
    history.node2 = boost::get<1>(node_pair);
    RagEdge<Region>* edge = rag.find_rag_edge(history.node1, history.node2);
    history.weight = edge->get_weight();
    history.preserve_edge = edge->is_preserve();
    history.false_edge = edge->is_false_edge();
    history.remove = false;
    history_queue.push_back(history);
    edge->set_weight(weight);
}

template <typename Region> void EdgePriority<Region>::clear_history()
{
    history_queue.clear();
}


template <typename Region> bool EdgePriority<Region>::undo(RagEdge<Region>** edge)
{
    if (history_queue.empty()) {
        return false;
    }
    EdgeHistory& history = history_queue.back();

    if (history.remove) {
        rag.remove_rag_node(rag.find_rag_node(history.node1));
    
        RagNode<Region>* temp_node1 = rag.insert_rag_node(history.node1);
        RagNode<Region>* temp_node2 = rag.insert_rag_node(history.node2);
        temp_node1->set_size(history.size1);
        temp_node2->set_size(history.size2);

        for (int i = 0; i < history.node_list1.size(); ++i) {
            RagEdge<Region>* temp_edge = rag.insert_rag_edge(temp_node1, rag.find_rag_node(history.node_list1[i]));
            temp_edge->set_weight(history.edge_weight1[i]);
            temp_edge->set_preserve(history.preserve_edge1[i]);
            temp_edge->set_false_edge(history.false_edge1[i]);
            rag_add_propertyptr(&rag, temp_edge, "location", history.property_list1[i][0]);
            rag_add_propertyptr(&rag, temp_edge, "edge_size", history.property_list1[i][1]);
        } 
        for (int i = 0; i < history.node_list2.size(); ++i) {
            RagEdge<Region>* temp_edge = rag.insert_rag_edge(temp_node2, rag.find_rag_node(history.node_list2[i]));
            temp_edge->set_weight(history.edge_weight2[i]);
            temp_edge->set_preserve(history.preserve_edge2[i]);
            temp_edge->set_false_edge(history.false_edge2[i]);
            rag_add_propertyptr(&rag, temp_edge, "location", history.property_list2[i][0]);
            rag_add_propertyptr(&rag, temp_edge, "edge_size", history.property_list2[i][1]);
        }

        RagEdge<Region>* temp_edge = rag.insert_rag_edge(temp_node1, temp_node2);
        temp_edge->set_weight(history.weight);
        temp_edge->set_preserve(history.preserve_edge); 
        temp_edge->set_false_edge(history.false_edge); 
        rag_add_propertyptr(&rag, temp_edge, "location", history.property_list_curr[0]);
        rag_add_propertyptr(&rag, temp_edge, "edge_size", history.property_list_curr[1]);
        
        for (typename NodePropertyMap::iterator iter = history.node_properties1.begin(); 
            iter != history.node_properties1.end();
                ++iter) {
            rag_add_propertyptr(&rag, temp_node1, iter->first, iter->second);
        }
        for (typename NodePropertyMap::iterator iter = history.node_properties2.begin();
            iter != history.node_properties2.end();
                ++iter) {
            rag_add_propertyptr(&rag, temp_node2, iter->first, iter->second);
        }
        if (edge) {
            *edge = temp_edge;
        }
    } else {
        RagNode<Region>* temp_node1 = rag.find_rag_node(history.node1);
        RagNode<Region>* temp_node2 = rag.find_rag_node(history.node2);

        RagEdge<Region>* temp_edge = rag.find_rag_edge(temp_node1, temp_node2);
        temp_edge->set_weight(history.weight);
        temp_edge->set_preserve(history.preserve_edge); 
        temp_edge->set_false_edge(history.false_edge); 
        if (edge) {
            *edge = temp_edge;
        }
        //rag_add_propertyptr(&rag, temp_edge, "location", history.property_list_curr[0]);
        //rag_add_propertyptr(&rag, temp_edge, "edge_size", history.property_list_curr[1]);
    }

    history_queue.pop_back();
    return true;
}


template <typename Region> void EdgePriority<Region>::removeEdge(NodePair node_pair, bool remove, std::vector<std::string>& node_properties)
{
    assert(remove);

    EdgeHistory history;
    history.node1 = boost::get<0>(node_pair);
    history.node2 = boost::get<1>(node_pair);
    RagEdge<Region>* edge = rag.find_rag_edge(history.node1, history.node2);
    history.weight = edge->get_weight();
    history.preserve_edge = edge->is_preserve();
    history.false_edge = edge->is_false_edge();
    history.property_list_curr.push_back(rag_retrieve_propertyptr(&rag, edge, "location"));
    history.property_list_curr.push_back(rag_retrieve_propertyptr(&rag, edge, "edge_size"));

    // node id that is kept
    history.remove = true;
    RagNode<Region>* node1 = rag.find_rag_node(history.node1);
    RagNode<Region>* node2 = rag.find_rag_node(history.node2);
    history.size1 = node1->get_size();
    history.size2 = node2->get_size();

    for (int i = 0; i < node_properties.size(); ++i) {
        std::string property_type = node_properties[i];
        try {
            boost::shared_ptr<Property> property = rag_retrieve_propertyptr(&rag, node1, property_type);
            history.node_properties1[property_type] = property;
        } catch(...) {
            //
        }
        try {
            boost::shared_ptr<Property> property = rag_retrieve_propertyptr(&rag, node2, property_type);
            history.node_properties2[property_type] = property;
        } catch(...) {
            //
        }
    }

    for (typename RagNode<Region>::edge_iterator iter = node1->edge_begin(); iter != node1->edge_end(); ++iter) {
        RagNode<Region>* other_node = (*iter)->get_other_node(node1);
        if (other_node != node2) {
            history.node_list1.push_back(other_node->get_node_id());
            history.edge_weight1.push_back((*iter)->get_weight());
            history.preserve_edge1.push_back((*iter)->is_preserve());
            history.false_edge1.push_back((*iter)->is_false_edge());


            std::vector<boost::shared_ptr<Property> > property_list;
            property_list.push_back(rag_retrieve_propertyptr(&rag, *iter, "location"));  
            property_list.push_back(rag_retrieve_propertyptr(&rag, *iter, "edge_size"));  
            history.property_list1.push_back(property_list);
        }
    } 

    for (typename RagNode<Region>::edge_iterator iter = node2->edge_begin(); iter != node2->edge_end(); ++iter) {
        RagNode<Region>* other_node = (*iter)->get_other_node(node2);
        if (other_node != node1) {
            history.node_list2.push_back(other_node->get_node_id());
            history.edge_weight2.push_back((*iter)->get_weight());
            history.preserve_edge2.push_back((*iter)->is_preserve());
            history.false_edge2.push_back((*iter)->is_false_edge());

            std::vector<boost::shared_ptr<Property> > property_list;
            property_list.push_back(rag_retrieve_propertyptr(&rag, *iter, "location"));  
            property_list.push_back(rag_retrieve_propertyptr(&rag, *iter, "edge_size"));  
            history.property_list2.push_back(property_list);
        }
    }

    rag_merge_edge(rag, edge, node1, property_names); 

    history_queue.push_back(history);
}






}

#endif

