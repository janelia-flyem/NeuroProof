#ifndef RAG_H
#define RAG_H

#include "RagEdge.h"
#include "RagNode.h"
#include "PropertyList.h"
#include "../Utilities/ErrMsg.h"
#include <tr1/unordered_set>
#include <tr1/unordered_map>

namespace NeuroProof {

template <typename Region>
class Rag {
  public:
    Rag();
    Rag(const Rag<Region>& dup_rag);
    Rag& operator=(const Rag<Region>& dup_rag);
    ~Rag();

    RagNode<Region>* find_rag_node(Region region);
    RagNode<Region>* insert_rag_node(Region region);
   
    RagEdge<Region>* find_rag_edge(Region region1, Region region2);
    RagEdge<Region>* find_rag_edge(RagNode<Region>* node1, RagNode<Region>* node2);
    RagEdge<Region>* insert_rag_edge(RagNode<Region>* rag_node1, RagNode<Region>* rag_node2);

    void remove_rag_node(RagNode<Region>* rag_node);
    void remove_rag_edge(RagEdge<Region>* rag_edge);

    size_t get_num_regions() const;
    size_t get_num_edges() const;
    
    typedef std::tr1::unordered_set<RagEdge<Region>*, RagEdgePtrHash<Region>, RagEdgePtrEq<Region> >  EdgeHash;
    typedef std::tr1::unordered_map<std::string, boost::shared_ptr<PropertyList<Region> > >  PropertyListMap;
    typedef std::tr1::unordered_set<RagNode<Region>*, RagNodePtrHash<Region>, RagNodePtrEq<Region> >  NodeHash;

    void bind_property_list(std::string property_type, boost::shared_ptr<PropertyList<Region> > property_list);
    boost::shared_ptr<PropertyList<Region> > retrieve_property_list(std::string property_type);
    void unbind_property_list(std::string property_type);

    class edges_iterator {
      public:
        typedef edges_iterator self_type;
        typedef RagEdge<Region>* value_type;
        typedef RagEdge<Region>* reference;
        typedef RagEdge<Region>** pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        edges_iterator();
        edges_iterator(typename EdgeHash::iterator edges_iter_) : edges_iter(edges_iter_) {}
        edges_iterator(const edges_iterator& iterator2) : edges_iter(iterator2.edges_iter) {}

        edges_iterator& operator=(const edges_iterator& iterator2)
        {
            edges_iter = iterator2.edges_iter;
            return *this;
        }
        edges_iterator& operator++()
        {
            edges_iter++;
            return *this;
        }
        edges_iterator operator++(int junk)
        {
            edges_iterator temp = *this;
            edges_iter++;
            return temp;
        }
        RagEdge<Region>* operator*()
        {
            return const_cast<RagEdge<Region>*>((*edges_iter));
        }
        RagEdge<Region>** operator->()
        {
            return &(const_cast<RagEdge<Region>*>(*edges_iter));
        }
        bool operator==(const edges_iterator& iterator2) const
        {
            return (edges_iter == iterator2.edges_iter);
        }
        bool operator!=(const edges_iterator& iterator2) const
        {
            return (edges_iter != iterator2.edges_iter);
        }
      private:
        typename EdgeHash::iterator edges_iter;
    };

    class nodes_iterator {
      public:
        typedef nodes_iterator self_type;
        typedef RagNode<Region>* value_type;
        typedef RagNode<Region>* reference;
        typedef RagNode<Region>** pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        nodes_iterator();
        nodes_iterator(typename NodeHash::iterator nodes_iter_) : nodes_iter(nodes_iter_) {}
        nodes_iterator(const nodes_iterator& iterator2) : nodes_iter(iterator2.nodes_iter) {}

        nodes_iterator& operator=(const nodes_iterator& iterator2)
        {
            nodes_iter = iterator2.nodes_iter;
            return *this;
        }
        nodes_iterator& operator++()
        {
            nodes_iter++;
            return *this;
        }
        nodes_iterator operator++(int junk)
        {
            nodes_iterator temp = *this;
            nodes_iter++;
            return temp;
        }
        RagNode<Region>* operator*()
        {
            return const_cast<RagNode<Region>*>(*nodes_iter);
        }
        RagNode<Region>** operator->()
        {
            return &(const_cast<RagNode<Region>*>(*nodes_iter));
        }
        bool operator==(const nodes_iterator& iterator2) const
        {
            return (nodes_iter == iterator2.nodes_iter);
        }
        bool operator!=(const nodes_iterator& iterator2) const
        {
            return (nodes_iter != iterator2.nodes_iter);
        }
      private:
        typename NodeHash::iterator nodes_iter;
    };
    nodes_iterator nodes_begin()
    {
        return nodes_iterator(rag_nodes.begin());
    }

    edges_iterator edges_begin()
    {
        return edges_iterator(rag_edges.begin());
    }
    nodes_iterator nodes_end()
    {
        return nodes_iterator(rag_nodes.end());
    }
    edges_iterator edges_end()
    {
        return edges_iterator(rag_edges.end());
    }

  private:
    // internal functions 
    void swap_em(Rag<Region>* rag_core);
    void init_probes();
    RagNode<Region>* find_rag_node(RagNode<Region>* node);
    RagEdge<Region>* find_rag_edge(RagEdge<Region>* edge);
    RagEdge<Region>* get_probe_edge(RagNode<Region>* rag_node1, RagNode<Region>* rag_node2);
    RagEdge<Region>* get_probe_edge(Region region1, Region region2);
    void remove_property(RagEdge<Region>* rag_edge);
    void remove_property(RagNode<Region>* rag_node);


    // core data lists
    EdgeHash rag_edges;
    NodeHash rag_nodes;
    PropertyListMap property_list_map;


    // internal probes for efficiency
    RagEdge<Region>* probe_rag_edge;
    RagNode<Region>* probe_rag_node;
    RagNode<Region>* probe_rag_node2;
};


// Rag constructors/destructor
template <typename Region> Rag<Region>::Rag()
{
    init_probes();    
} 

template <typename Region> Rag<Region>::Rag(const Rag<Region>& dup_rag)
{
    init_probes();
    
    for (typename EdgeHash::const_iterator iter = dup_rag.rag_edges.begin(); iter != dup_rag.rag_edges.end(); ++iter) {
        RagEdge<Region>* rag_edge = new RagEdge<Region>(**iter);
        rag_edges.insert(rag_edge);
    }
    for (typename NodeHash::const_iterator iter = dup_rag.rag_nodes.begin(); iter != dup_rag.rag_nodes.end(); ++iter) {
        RagNode<Region>* rag_node = new RagNode<Region>(**iter);
        rag_nodes.insert(rag_node);
    }
    for (typename EdgeHash::iterator iter = rag_edges.begin(); iter != rag_edges.end(); ++iter) {
        RagEdge<Region>* curr_edge = const_cast<RagEdge<Region>*>(*iter); 
        RagNode<Region>* temp_node1 = curr_edge->get_node1();
        RagNode<Region>* temp_node2 = curr_edge->get_node2();
        temp_node1 = find_rag_node(temp_node1);
        assert(temp_node1);
        temp_node2 = find_rag_node(temp_node2);
        assert(temp_node2);
        curr_edge->set_edge(temp_node1, temp_node2);
    }
    for (typename NodeHash::const_iterator iter = dup_rag.rag_nodes.begin(); iter != dup_rag.rag_nodes.end(); ++iter) {
        RagNode<Region> * scan_node = const_cast<RagNode<Region>*>(*iter);
        RagNode<Region> * curr_node = find_rag_node(scan_node);
    

        for (typename RagNode<Region>::edge_iterator edge_iter = scan_node->edge_begin(); edge_iter != scan_node->edge_end(); ++edge_iter) {
            curr_node->remove_edge(*edge_iter);
            curr_node->insert_edge(find_rag_edge(*edge_iter));
        }
    }

    for (typename PropertyListMap::const_iterator iter = dup_rag.property_list_map.begin(); iter != dup_rag.property_list_map.end(); ++iter) {
        property_list_map[iter->first] = iter->second->copy(); 
    }

}

template <typename Region> Rag<Region>& Rag<Region>::operator=(const Rag<Region>& dup_rag)
{
    if (this !=  &dup_rag) {
        Rag<Region> rag_temp(dup_rag);
        swap_em(&rag_temp);
    }
    return *this;
}

template <typename Region> Rag<Region>::~Rag()
{
    delete probe_rag_edge;
    delete probe_rag_node;
    delete probe_rag_node2;
    for (typename EdgeHash::iterator iter = rag_edges.begin(); iter != rag_edges.end(); ++iter) {
        delete *iter;
    }
    for (typename NodeHash::iterator iter = rag_nodes.begin(); iter != rag_nodes.end(); ++iter) {
        delete *iter;
    }
}


// support functions
template <typename Region> void Rag<Region>::init_probes()
{
    probe_rag_node = RagNode<Region>::New(Region());
    probe_rag_node2 = RagNode<Region>::New(Region());
    probe_rag_edge = RagEdge<Region>::New(probe_rag_node, probe_rag_node2);
}

template <typename Region> inline RagNode<Region>* Rag<Region>::insert_rag_node(Region region)
{
    RagNode<Region>* node = RagNode<Region>::New(region);

    if (rag_nodes.find(node) != rag_nodes.end()) {
        throw ErrMsg("Reinserting a node into the Rag");
    } 
    rag_nodes.insert(node);
    return node;
}
template <typename Region> inline RagEdge<Region>* Rag<Region>::insert_rag_edge(RagNode<Region>* rag_node1, RagNode<Region>* rag_node2)
{
    RagEdge<Region>* edge = RagEdge<Region>::New(rag_node1, rag_node2);
    
    if (rag_edges.find(edge) != rag_edges.end()) {
        throw ErrMsg("Reinserting an edge into the Rag");
    } 
    
    rag_edges.insert(edge);
    rag_node1->insert_edge(edge);
    rag_node2->insert_edge(edge);
    return edge;
}

template <typename Region> inline RagNode<Region>* Rag<Region>::find_rag_node(Region region)
{
    probe_rag_node->set_node_id(region);
    RagNode<Region>* rag_node = 0;
    typename NodeHash::iterator rag_node_iter = rag_nodes.find(probe_rag_node);
    if (rag_node_iter != rag_nodes.end()) {
        rag_node = *rag_node_iter;
    }
    return rag_node;
}
template <typename Region> inline RagNode<Region>* Rag<Region>::find_rag_node(RagNode<Region>* node)
{
    RagNode<Region>* rag_node = 0;
    typename NodeHash::iterator rag_node_iter = rag_nodes.find(node);
    if (rag_node_iter != rag_nodes.end()) {
        rag_node = *rag_node_iter;
    }
    return rag_node;
}


template <typename Region> inline RagEdge<Region>* Rag<Region>::get_probe_edge(Region region1, Region region2)
{
    probe_rag_node->set_node_id(region1);
    probe_rag_node2->set_node_id(region2);
    probe_rag_edge->set_edge(probe_rag_node, probe_rag_node2);
    return probe_rag_edge;
}

template <typename Region> inline RagEdge<Region>* Rag<Region>::get_probe_edge(RagNode<Region>* rag_node1, RagNode<Region>* rag_node2)
{
    probe_rag_edge->set_edge(rag_node1, rag_node2);
    return probe_rag_edge;
}

template <typename Region> inline RagEdge<Region>* Rag<Region>::find_rag_edge(Region region1, Region region2)
{
    RagEdge<Region>* rag_edge = 0;
    typename EdgeHash::iterator rag_edge_iter = rag_edges.find(get_probe_edge(region1, region2));
    if (rag_edge_iter != rag_edges.end()) {
        rag_edge = *rag_edge_iter;
    }
    return rag_edge;
}


template <typename Region> inline RagEdge<Region>* Rag<Region>::find_rag_edge(RagNode<Region>* node1, RagNode<Region>* node2)
{
    RagEdge<Region>* rag_edge = 0;
    typename EdgeHash::iterator rag_edge_iter = rag_edges.find(get_probe_edge(node1, node2));
    if (rag_edge_iter != rag_edges.end()) {
        rag_edge = *rag_edge_iter;
    }
    return rag_edge;
}

template <typename Region> inline RagEdge<Region>* Rag<Region>::find_rag_edge(RagEdge<Region>* edge)
{
    RagEdge<Region>* rag_edge = 0;
    typename EdgeHash::iterator rag_edge_iter = rag_edges.find(edge);
    if (rag_edge_iter != rag_edges.end()) {
        rag_edge = *rag_edge_iter;
    }
    return rag_edge;
}


template <typename Region> inline void Rag<Region>::remove_rag_node(RagNode<Region>* rag_node)
{
    typename NodeHash::iterator rag_node_iter = rag_nodes.find(rag_node);
    if (rag_node_iter == rag_nodes.end()) {
        throw ErrMsg("edge does not exist");
    }

    std::vector<RagEdge<Region>* > edge_list;
    for (typename RagNode<Region>::edge_iterator iter = rag_node->edge_begin(); iter != rag_node->edge_end(); ++iter) {
        edge_list.push_back(*iter);
    }

    for (int i = 0; i < edge_list.size(); ++i) {
        edge_list[i]->get_node1()->remove_edge(edge_list[i]); 
        edge_list[i]->get_node2()->remove_edge(edge_list[i]); 
        
        remove_property(edge_list[i]);
        rag_edges.erase(edge_list[i]);
    }

    remove_property(rag_node);
    rag_nodes.erase(rag_node);
}

template <typename Region> inline void Rag<Region>::remove_rag_edge(RagEdge<Region>* rag_edge)
{
    typename EdgeHash::iterator rag_edge_iter = rag_edges.find(rag_edge);
    if (rag_edge_iter == rag_edges.end()) {
        throw ErrMsg("edge does not exist");
    }
    rag_edge->get_node1()->remove_edge(rag_edge);
    rag_edge->get_node2()->remove_edge(rag_edge);

    remove_property(rag_edge);
    rag_edges.erase(rag_edge);
}


template <typename Region> inline void Rag<Region>::remove_property(RagEdge<Region>* rag_edge)
{
    for (typename PropertyListMap::iterator iter = property_list_map.begin(); iter != property_list_map.end(); ++iter) { 
        try {
            iter->second->remove_property(rag_edge);
        } catch (ErrMsg& msg) {
            //
        }
    }
}
template <typename Region> inline void Rag<Region>::remove_property(RagNode<Region>* rag_node)
{
    for (typename PropertyListMap::iterator iter = property_list_map.begin(); iter != property_list_map.end(); ++iter) { 
        try {
            iter->second->remove_property(rag_node);
        } catch (ErrMsg& msg) {
            //
        }
    }
}


template <typename Region> void Rag<Region>::bind_property_list(std::string property_type, boost::shared_ptr<PropertyList<Region> > property_list)
{
    property_list_map[property_type] = property_list;
} 

template <typename Region> inline boost::shared_ptr<PropertyList<Region> > Rag<Region>::retrieve_property_list(std::string property_type)
{
    typename PropertyListMap::iterator iter = property_list_map.find(property_type);
    if (iter == property_list_map.end()) {
        throw ErrMsg(std::string("No properties loaded for ") + property_type);
    }
    return iter->second;
} 

template <typename Region> inline void Rag<Region>::unbind_property_list(std::string property_type)
{
    property_list_map.erase(property_type);
} 



template <typename Region> inline size_t Rag<Region>::get_num_regions() const
{
    return rag_nodes.size();
}

template <typename Region> inline size_t Rag<Region>::get_num_edges() const
{
    return rag_edges.size();
} 

template <typename Region> void Rag<Region>::swap_em(Rag<Region>* rag_core)
{
    std::swap(rag_edges, rag_core->rag_edges);
    std::swap(rag_nodes, rag_core->rag_nodes);
    std::swap(property_list_map, rag_core->property_list_map);
    std::swap(probe_rag_edge, rag_core->probe_rag_edge);
    std::swap(probe_rag_node, rag_core->probe_rag_node);
    std::swap(probe_rag_node2, rag_core->probe_rag_node2);
}

}

#endif
