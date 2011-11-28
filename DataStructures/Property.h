#ifndef PROPERTY_H
#define PROPERTY_H

#include <boost/shared_ptr.hpp>
#include "../Utilities/ErrMsg.h"
#include "AffinityPair.h"
#include "RagEdge.h"
#include "RagNode.h"
#include <tr1/unordered_map>

namespace NeuroProof {

class Property {
  public:
    virtual boost::shared_ptr<Property> copy() = 0;
    // add serialization/deserialization interface
};

template <typename T>
class PropertyTemplate : public Property {
  public:
    PropertyTemplate(T data_) : Property(), data(data_) {}
    virtual boost::shared_ptr<Property> copy()
    {
        return boost::shared_ptr<Property>(new PropertyTemplate<T>(*this));
    }
    virtual T& get_data()
    {
        return data;
    }
    // add serialization/deserialization interface -- just memory copy

  protected:
    T data;
};


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


}


#endif
