#ifndef PROPERTYLIST_H
#define PROPERTYLIST_H

#include "Property.h"
#include <iostream>

namespace NeuroProof {

template <typename Region>
class PropertyList {
  public:
    virtual boost::shared_ptr<PropertyList> copy() = 0;
    
    virtual void add_property(RagEdge<Region>* rag_edge, boost::shared_ptr<Property> property)
    {
        throw ErrMsg("Not an edge property");
    }
    virtual void add_property(RagNode<Region>* rag_node, boost::shared_ptr<Property> property)
    {
        throw ErrMsg("Not a node property");
    }
    virtual boost::shared_ptr<Property> retrieve_property(RagEdge<Region>* rag_edge)
    {
        throw ErrMsg("Not an edge property");
    }
    virtual boost::shared_ptr<Property> retrieve_property(RagNode<Region>* rag_node)
    {
        throw ErrMsg("Not a node property");
    }
    virtual void remove_property(RagEdge<Region>* rag_edge)
    {
        throw ErrMsg("Not an edge property"); 
    }
    virtual void remove_property(RagNode<Region>* rag_node)
    {
        throw ErrMsg("Not a node property");
    }
};


template <typename Region>
class NodePropertyList : public PropertyList<Region> {
  public:
    static boost::shared_ptr<NodePropertyList<Region> > create_node_list()
    {
        return boost::shared_ptr<NodePropertyList<Region> >(new NodePropertyList<Region>);
    }
    boost::shared_ptr<PropertyList<Region> > copy()
    {
        NodePropertyList* temp_list = new NodePropertyList<Region>;
        temp_list->properties = properties;

        for (typename PropertyList_t::iterator iter = temp_list->properties.begin(); iter != temp_list->properties.end(); ++iter) {
            iter->second = iter->second->copy();
        }
        return boost::shared_ptr<PropertyList<Region> >(temp_list);
    }
    virtual void add_property(RagNode<Region>* rag_node, boost::shared_ptr<Property> property)
    {
        properties[rag_node->get_node_id()] = property;
    }
    virtual boost::shared_ptr<Property> retrieve_property(RagNode<Region>* rag_node)
    {
        typename PropertyList_t::iterator iter = properties.find(rag_node->get_node_id());
        if (iter != properties.end()) {
            return iter->second;
        } else {
            throw ErrMsg("Property does not exist for the given node");
        }
    }
    virtual void remove_property(RagNode<Region>* rag_node)
    {
        properties.erase(rag_node->get_node_id()); 
    }

  private:
    typedef std::tr1::unordered_map<Region, boost::shared_ptr<Property> > PropertyList_t;
    PropertyList_t properties;
};

template <typename Region>
class EdgePropertyList : public PropertyList<Region> {
  public:
    static boost::shared_ptr<EdgePropertyList<Region> > create_edge_list()
    {
        return boost::shared_ptr<EdgePropertyList<Region> >(new EdgePropertyList<Region>);
    }
    boost::shared_ptr<PropertyList<Region> > copy()
    {
        EdgePropertyList* temp_list = new EdgePropertyList<Region>;
        temp_list->properties = properties;

        for (typename PropertyList_t::iterator iter = temp_list->properties.begin(); iter != temp_list->properties.end(); ++iter) {
            iter->second = iter->second->copy();
        }
        return boost::shared_ptr<PropertyList<Region> >(temp_list);
    }
    virtual void add_property(RagEdge<Region>* rag_edge, boost::shared_ptr<Property> property)
    {
        properties[OrderedPair<Region>(rag_edge->get_node1()->get_node_id(), rag_edge->get_node2()->get_node_id())] = property;
    }
    virtual boost::shared_ptr<Property> retrieve_property(RagEdge<Region>* rag_edge)
    {
        typename PropertyList_t::iterator iter = properties.find(OrderedPair<Region>(rag_edge->get_node1()->get_node_id(), rag_edge->get_node2()->get_node_id()));
        if (iter != properties.end()) {
            return iter->second;
        } else {
            throw ErrMsg("Property does not exist for the given edge");
        }
    }
    virtual void remove_property(RagEdge<Region>* rag_edge)
    {
        properties.erase(OrderedPair<Region>(rag_edge->get_node1()->get_node_id(), rag_edge->get_node2()->get_node_id())); 
    }


  private:
    typedef std::tr1::unordered_map<OrderedPair<Region>, boost::shared_ptr<Property>, OrderedPair<Region> > PropertyList_t;
    PropertyList_t properties;
};

// helper functions

template <typename Region, typename T> 
T property_list_retrieve_template_property(boost::shared_ptr<PropertyList<Region> > property_list, RagNode<Region>* node)
{
    boost::shared_ptr<PropertyTemplate<T> > property = boost::shared_polymorphic_downcast<PropertyTemplate<T> >(property_list->retrieve_property(node));
    return property->get_data();
}

template <typename Region, typename T> 
T property_list_retrieve_template_property(boost::shared_ptr<PropertyList<Region> > property_list, RagEdge<Region>* edge)
{
    boost::shared_ptr<PropertyTemplate<T> > property = boost::shared_polymorphic_downcast<PropertyTemplate<T> >(property_list->retrieve_property(edge));
    return property->get_data();
}

template <typename Region, typename T> 
boost::shared_ptr<PropertyTemplate<T> > property_list_retrieve_template_property_ptr(boost::shared_ptr<PropertyList<Region> > property_list, RagNode<Region>* node)
{
    return boost::shared_polymorphic_downcast<PropertyTemplate<T> >(property_list->retrieve_property(node));
}

template <typename Region, typename T> 
boost::shared_ptr<PropertyTemplate<T> > property_list_retrieve_template_property_ptr(boost::shared_ptr<PropertyList<Region> > property_list, RagEdge<Region>* edge)
{
    return boost::shared_polymorphic_downcast<PropertyTemplate<T> >(property_list->retrieve_property(edge));
}

template <typename Region, typename T> 
void property_list_add_template_property(boost::shared_ptr<PropertyList<Region> > property_list, RagNode<Region>* node, T data)
{
    boost::shared_ptr<Property> property(new PropertyTemplate<T>(data));
    property_list->add_property(node, property);
}

template <typename Region, typename T> 
void property_list_add_template_property(boost::shared_ptr<PropertyList<Region> > property_list, RagEdge<Region>* edge, T data)
{
    boost::shared_ptr<Property> property(new PropertyTemplate<T>(data));
    property_list->add_property(edge, property);
}
    
    
    
    
    
}


#endif
