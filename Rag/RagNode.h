/*
 * Interface and definition of the node element that is a fundamental
 * unit of the Region Adjancency Graph (RAG).
 * 
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/


#ifndef RAGNODE_H
#define RAGNODE_H

#include "RagElement.h"
#include <vector>
#include <algorithm>
#include <map>
#include <set>

#define BOUNDARY_SIZE "boundary-size"

namespace NeuroProof {

// forward declaration of RagEdge
template <typename Region>
class RagEdge;

/*!
 * Fundamental node element used in the RAG.  It is expected that each
 * node is created on the heap.  Pointers with global scope is 
 * is necessary for traversing edges and other nodes.  User-defined
 * datatypes can be used as the fundamental data stored at the node.
 * Operators must be overload to handle the operations used in this class.
 * Inherits property interface from RagElement
*/ 
template <typename Region>
class RagNode : public RagElement {
  public:
    //! Static creation of nodes on the heap only
    static RagNode<Region>* New(Region node_int)
    {
        return new RagNode(node_int);
    }

    /*!
     * Retrieve the main Region element/id associated with this node 
     * \return Region associated with this node
    */
    Region get_node_id() const;

    /*!
     * Retrieve size associated with the current node
     * \return Size of node
    */
    unsigned long long get_size() const;
    
    /*!
     * Set the size of the node
     * \param size_ the size desired for node
    */ 
    void set_size(unsigned long long size_);
    
   
    // TODO: remove from interface 
    void set_boundary_size(unsigned long long size_)
    {
        set_property(BOUNDARY_SIZE, size_);
    }

    /*!
     * Reset the Region element/id associated with the node
     * \param region a region type
    */ 
    void set_node_id(Region region);

    /*!
     * Increase the size of the node by certain increment
     * \param incr size of the increment
    */ 
    void incr_size(unsigned long long incr = 1)
    {
        size += incr;
    }

    // TODO: remove from interface
    void incr_boundary_size(unsigned long long incr = 1)
    {
        unsigned long long boundary_size = 
            get_property<unsigned long long>(BOUNDARY_SIZE);
        set_property(BOUNDARY_SIZE, (boundary_size + incr));
    }

    // TODO: remove from 
    unsigned long long get_boundary_size()
    {
        return get_property<unsigned long long>(BOUNDARY_SIZE);
    }

    /*!
     * Determined the number of nodes immediately connected to this node
     * \return the degree of the node
    */ 
    size_t node_degree() const;
    
    /*!
     * Adds pointer to an edge to the node.  Nodes can have >=0 edges.
     * \param edge pointer to rag edge
    */ 
    void insert_edge(RagEdge<Region>* edge);
    
    /*!
     * Removes pointer to edge from list of node edges
     * \param edge pointer to rag edge
    */ 
    void remove_edge(RagEdge<Region>* edge);
    
    unsigned long long compute_border_length();
 	

    // overloaded operators for user convenience
    bool operator<(const RagNode<Region>& node2) const;
    bool operator==(const RagNode<Region>& node2) const;
    size_t operator()(const RagNode<Region>& node) const;

    // iterator access to the nodes edges
    typedef std::vector<RagEdge<Region>* > RagEdgeList; 
    class edge_iterator {
      public:
        typedef edge_iterator self_type;
        typedef RagEdge<Region>* value_type;
        typedef RagEdge<Region>* reference;
        typedef RagEdge<Region>** pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        edge_iterator();
        edge_iterator(typename RagEdgeList::iterator ragnode_iter_) : ragnode_iter(ragnode_iter_) {}
        edge_iterator(const edge_iterator& iterator2) : ragnode_iter(iterator2.ragnode_iter) {}
        
        edge_iterator& operator=(const edge_iterator& iterator2)
        {
            ragnode_iter = iterator2.ragnode_iter;
            return *this;
        }
        edge_iterator& operator++()
        {
            ragnode_iter++;
            return *this;
        }
        edge_iterator operator++(int junk)
        {
            edge_iterator temp = *this;
            ragnode_iter++;
            return temp;
        }
        RagEdge<Region>* operator*()
        {
            return const_cast<RagEdge<Region>* >(*ragnode_iter);
        }
        RagEdge<Region>** operator->()
        {
            return &(const_cast<RagEdge<Region>* >(*ragnode_iter));
        }
        bool operator==(const edge_iterator& iterator2) const
        {
            return (ragnode_iter == iterator2.ragnode_iter);
        }
        bool operator!=(const edge_iterator& iterator2) const
        {
            return (ragnode_iter != iterator2.ragnode_iter);
        }
      private:
        typename RagEdgeList::iterator ragnode_iter;

    };

    class node_iterator {
      public:
          typedef node_iterator self_type;
          typedef RagNode<Region>* value_type;
          typedef RagNode<Region>* reference;
          typedef RagNode<Region>** pointer;
          typedef std::forward_iterator_tag iterator_category;
          typedef int difference_type;

        node_iterator();
        node_iterator(RagNode<Region>* rag_node_, typename RagEdgeList::iterator ragnode_iter_) : rag_node(rag_node_), ragnode_iter(ragnode_iter_) {}
        node_iterator(const node_iterator& iterator2) : ragnode_iter(iterator2.ragnode_iter), rag_node(iterator2.rag_node) {}
        
        node_iterator& operator=(const node_iterator& iterator2)
        {
            ragnode_iter = iterator2.ragnode_iter;
            rag_node = iterator2.rag_node;
            return *this;
        }
        node_iterator& operator++()
        {
            ragnode_iter++;
            return *this;
        }
        node_iterator operator++(int junk)
        {
            node_iterator temp = *this;
            ragnode_iter++;
            return temp;
        }

        RagNode<Region>* operator*()
        { 
            RagNode<Region> * temp =  (*ragnode_iter)->get_node1();
            if (temp != rag_node) {
                return temp;
            } else {
                return ((*ragnode_iter)->get_node2());
            }
        }
        RagNode<Region>** operator->()
        {
            RagNode<Region> * temp =  (*ragnode_iter)->get_node1();
            if (temp != rag_node) {
                return &temp;
            } else {
                return &((*ragnode_iter)->get_node2());
            }
        }
        bool operator==(const node_iterator& iterator2) const
        {
            return (ragnode_iter == iterator2.ragnode_iter);
        }
        bool operator!=(const node_iterator& iterator2) const
        {
            return (ragnode_iter != iterator2.ragnode_iter);
        }
      private:
        typename RagEdgeList::iterator ragnode_iter;
        RagNode<Region>* rag_node;
    };
    edge_iterator edge_begin()
    {
        return edge_iterator(edges.begin());
    }

    edge_iterator edge_end()
    {
        return edge_iterator(edges.end());
    }

    node_iterator node_begin()
    {
        return node_iterator(this, edges.begin());
    }

    node_iterator node_end()
    {
        return node_iterator(this, edges.end());
    }
 
    bool is_boundary()
    {
        return (get_property<unsigned long long>(BOUNDARY_SIZE) != 0);
    }


  private:

    RagNode<Region>&  operator=(const RagNode<Region>& node2) {}
    RagNode(Region node_int_) : size(0), node_int(node_int_)
    {
        set_property(BOUNDARY_SIZE, (unsigned long long)(0));
    }
    
    RagEdgeList edges;
    unsigned long long size;
    Region node_int;
};

typedef unsigned int Node_uit;
typedef RagNode<Node_uit> RagNode_uit; 

template<typename Region> unsigned long long RagNode<Region>::compute_border_length()  {
    unsigned long long count = 0;
    for (RagNode<Region>::edge_iterator iter = this->edge_begin(); iter != this->edge_end(); ++iter) {
        if ((*iter)->is_false_edge()) {
            continue;
        }
        count += (*iter)->get_size();
    }
    count += (get_boundary_size());

    return count;

}


template<typename Region> inline Region RagNode<Region>::get_node_id() const
{
    return node_int;
}

template<typename Region> inline unsigned long long RagNode<Region>::get_size() const
{
    return size;
}

template<typename Region> inline void RagNode<Region>::set_size(unsigned long long size_)
{
    size = size_;
}

template<typename Region> inline void RagNode<Region>::set_node_id(Region region)
{
    node_int = region;
}

template<typename Region> size_t RagNode<Region>::node_degree() const
{
    return edges.size();
}


template<typename Region> inline void RagNode<Region>::insert_edge(RagEdge<Region>* edge)
{
    edges.push_back(edge);
}

template<typename Region> inline void RagNode<Region>::remove_edge(RagEdge<Region>* edge)
{
    edges.erase(std::remove(edges.begin(), edges.end(), edge), edges.end());
}


// overloaded oeprators
template<typename Region> inline bool RagNode<Region>::operator<(const RagNode<Region>& node2) const
{
    return (node_int < node2.node_int); 
}

template<typename Region> inline bool RagNode<Region>::operator==(const RagNode<Region>& node2) const
{
    return (node_int == node2.node_int);
}

template<typename Region> inline size_t RagNode<Region>::operator()(const RagNode<Region>& node) const
{
    return node.node_int; 
}


// Functors for comparisons of RagNodes
template<typename Region>
struct RagNodePtrEq {
    bool operator() (const RagNode<Region>* node1, const RagNode<Region>* node2) const
    {
        return (node1->get_node_id() == node2->get_node_id());
    }
};

template<typename Region>
struct RagNodePtrCmp {
    bool operator() (const RagNode<Region>* node1, const RagNode<Region>* node2) const
    {
        return (node1->get_node_id() < node2->get_node_id());
    }

};

template<typename Region>
struct RagNodePtrHash {
    size_t operator() (const RagNode<Region>* node) const
    {
        return  node->get_node_id();
    }
};

}

#endif
