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

template <typename Region, typename T>
void rag_add_property(Rag<Region>* rag, RagEdge<Region>* edge, std::string property_type, T data)
{
    boost::shared_ptr<PropertyList<Region> > property_list = rag->retrieve_property_list(property_type);
    boost::shared_ptr<Property> property(new PropertyTemplate<T>(data));
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

template <typename Region>
typename EdgeRanking<Region>::type rag_grab_edge_ranking(Rag<Region>& rag, double min_val, double max_val, double start_val)
{
    typename EdgeRanking<Region>::type ranking;

    for (typename Rag<Region>::edges_iterator iter = rag.edges_begin(); iter != rag.edges_end(); ++iter) {
        double val = (*iter)->get_weight();
        if ((val <= max_val) && (val >= min_val)) {
            if (rag_retrieve_property<Region, unsigned int>(&rag, *iter, "edge_size") > 1) {
            if (((*iter)->get_node1()->get_size() > 27) && ((*iter)->get_node2()->get_size() > 27)) {
                ranking.insert(std::make_pair(std::abs(val - start_val), *iter));
            }
            }
        } 
    }

    return ranking;
}



}

#endif
