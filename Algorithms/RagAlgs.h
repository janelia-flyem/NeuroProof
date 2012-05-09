#ifndef RAGALGS_H
#define RAGALGS_H

#include "../DataStructures/Rag.h"
#include "../DataStructures/Property.h"
#include <map>
#include <string>

namespace NeuroProof {

template <typename Region>
struct EdgeRanking {
    typedef std::multimap<double, RagEdge<Region>* > type;
};



template <typename Region>
void rag_bind_edge_property_list(Rag<Region>* rag, std::string property_type)
{
   rag->bind_property_list(property_type, EdgePropertyList<Region>::create_edge_list());
}

template <typename Region>
void rag_bind_node_property_list(Rag<Region>* rag, std::string property_type)
{
   rag->bind_property_list(property_type, NodePropertyList<Region>::create_node_list());
}

template <typename Region>
void rag_unbind_property_list(Rag<Region>* rag, std::string property_type)
{
   rag->unbind_property_list(property_type);
}

template <typename Region>
void rag_add_edge(Rag<Region>* rag, unsigned int id1, unsigned int id2, double pred, 
        boost::shared_ptr<PropertyList<Region> > edge_list)
{
    RagNode<Region> * node1 = rag->find_rag_node(id1);
    if (!node1) {
        node1 = rag->insert_rag_node(id1);
    }
    
    RagNode<Region> * node2 = rag->find_rag_node(id2);
    if (!node2) {
        node2 = rag->insert_rag_node(id2);
    }
    
    RagEdge<Region>* edge = rag->find_rag_edge(node1, node2);
    if (!edge) {
        edge = rag->insert_rag_edge(node1, node2);
        edge_list->add_property(edge, boost::shared_ptr<Property>(new PropertyMedian));
    }

    boost::shared_ptr<PropertyCompute> property = boost::shared_polymorphic_downcast<PropertyCompute>(edge_list->retrieve_property(edge));
    property->add_point(pred);
}  


template <typename Region, typename T>
void rag_add_property(Rag<Region>* rag, RagEdge<Region>* edge, std::string property_type, T data)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    boost::shared_ptr<Property> property(new PropertyTemplate<T>(data));
    property_list->add_property(edge, property);
}

template <typename Region>
void rag_add_propertyptr(Rag<Region>* rag, RagEdge<Region>* edge, std::string property_type, boost::shared_ptr<Property> property)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    property_list->add_property(edge, property);
}


template <typename Region, typename T>
void rag_add_property(Rag<Region>* rag, RagNode<Region>* node, std::string property_type, T data)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    boost::shared_ptr<Property> property(new PropertyTemplate<T>(data));
    property_list->add_property(node, property);
}

template <typename Region, typename T>
T rag_retrieve_property(Rag<Region>* rag, RagEdge<Region>* edge, std::string property_type)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    boost::shared_ptr<PropertyTemplate<T> > property = boost::shared_polymorphic_downcast<PropertyTemplate<T> >(property_list->retrieve_property(edge));
    return property->get_data();
}

template <typename Region, typename T>
T rag_retrieve_property(Rag<Region>* rag, RagNode<Region>* node, std::string property_type)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    boost::shared_ptr<PropertyTemplate<T> > property = boost::shared_polymorphic_downcast<PropertyTemplate<T> >(property_list->retrieve_property(node));
    return property->get_data();
}

template <typename Region>
boost::shared_ptr<Property> rag_retrieve_propertyptr(Rag<Region>* rag, RagEdge<Region>* edge, std::string property_type)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    return property_list->retrieve_property(edge);
}


template <typename Region>
void rag_remove_property(Rag<Region>* rag, RagEdge<Region>* edge, std::string property_type)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    property_list->remove_property(edge);
}

template <typename Region>
void rag_remove_property(Rag<Region>* rag, RagNode<Region>* node, std::string property_type)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    property_list->remove_property(node);
}

// take smallest value edge and use that when connecting back
template <typename Region>
void rag_merge_edge(Rag<Region>& rag, RagEdge<Region>* edge, RagNode<Region>* node_keep, std::vector<std::string>& property_names)
{
    RagNode<Region>* node_remove = edge->get_other_node(node_keep);

    for(typename RagNode<Region>::edge_iterator iter = node_remove->edge_begin(); iter != node_remove->edge_end(); ++iter) {
        double weight = (*iter)->get_weight();
        RagNode<Region>* other_node = (*iter)->get_other_node(node_remove);
        if (other_node == node_keep) {
            continue;
        }

        RagEdge<Region>* temp_edge = rag.find_rag_edge(node_keep, other_node);
        
        if (temp_edge && (weight < temp_edge->get_weight())) { 
            temp_edge->set_weight(weight);
            for (int i = 0; i < property_names.size(); ++i) {
                boost::shared_ptr<Property> property = rag_retrieve_propertyptr(&rag, *iter, property_names[i]);
                rag_add_propertyptr(&rag, temp_edge, property_names[i], property);
            }
        } else if (!temp_edge) {
            RagEdge<Region>* new_edge = rag.insert_rag_edge(node_keep, other_node);
            new_edge->set_weight(weight);
            for (int i = 0; i < property_names.size(); ++i) {
                boost::shared_ptr<Property> property = rag_retrieve_propertyptr(&rag, *iter, property_names[i]);
                rag_add_propertyptr(&rag, new_edge, property_names[i], property);
            }
         }
    }

    node_keep->set_size(node_keep->get_size() + node_remove->get_size());
    rag.remove_rag_node(node_remove);     
}

// assume that 0 body will never be added as a screen
template <typename Region>
typename EdgeRanking<Region>::type rag_grab_edge_ranking(Rag<Region>& rag, double min_val, double max_val, double start_val, int ignore_size=27, Region screen_id = 0)
{
    typename EdgeRanking<Region>::type ranking;

    for (typename Rag<Region>::edges_iterator iter = rag.edges_begin(); iter != rag.edges_end(); ++iter) {
        if ((screen_id != 0) && ((*iter)->get_node1()->get_node_id() != screen_id) && ((*iter)->get_node2()->get_node_id() != screen_id)) {
            continue;
        }
        
        double val = (*iter)->get_weight();
        if ((val <= max_val) && (val >= min_val)) {
//            if (rag_retrieve_property<Region, unsigned int>(&rag, *iter, "edge_size") > 1) {
            if (((*iter)->get_node1()->get_size() > ignore_size) && ((*iter)->get_node2()->get_size() > ignore_size)) {
    //        if (((*iter)->get_node1()->node_degree() > 1) && ((*iter)->get_node2()->node_degree() > 1)) {
                ranking.insert(std::make_pair(std::abs(val - start_val), *iter));
     //       }
  //          }
            }
        } 
    }

    return ranking;
}



}

#endif
