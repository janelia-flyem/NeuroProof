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

class PropertyCompute : public Property {
  public:
    virtual boost::shared_ptr<Property> copy() = 0;
    virtual double get_data() = 0;
    virtual void add_point(double val, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0) = 0;
};

#define NUM_BINS 101

class PropertyMedian : public PropertyCompute {
  public:
    PropertyMedian() : count(0)
    {
        for (int i = 0; i < NUM_BINS; ++i) {
            hist[i] = 0;
        }
    }
    boost::shared_ptr<Property> copy()
    {
        return boost::shared_ptr<Property>(new PropertyMedian(*this));
    }    
    void add_point(double val, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
    {
        hist[int(val * 100 + 0.5)]++;
        ++count;   
    }
    double get_data()
    {
        int mid_point1 = count / 2;
        int mid_point2 = (count + 1)/ 2;
    
        int spot1 = -1;
        int spot2 = -1;
        int curr_count = 0;
        for (int i = 0; i < NUM_BINS; ++i) {
            curr_count += hist[i];
            
            if (curr_count >= mid_point1 && spot1 == -1) {
                spot1 = i;   
            }
            if (curr_count >= mid_point2) {
                spot2 = i;
                break; 
            }
        }
        double median_spot = (double(spot1) * spot2) / 2;
        return (median_spot /= 100);
    }
        
  private:
    int count;
    int hist[NUM_BINS];
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
