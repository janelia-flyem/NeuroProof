#ifndef RAGNODE_H
#define RAGNODE_H

#include "RagEdge.h"
#include <vector>
#include <algorithm>

namespace NeuroProof {

template <typename Region>
class RagEdge;

template <typename Region>
class RagNode {
  public:
    static RagNode<Region>* New(Region node_int)
    {
        return new RagNode(node_int);
    }

    // node information
    Region get_node_id() const;
    unsigned long long get_size() const;
    void set_size(unsigned long long size_);
    void set_node_id(Region region);

    void incr_size()
    {
        ++size;
    }

    size_t node_degree() const;
    void insert_edge(RagEdge<Region>* edge);
    void remove_edge(RagEdge<Region>* edge);
    RagEdge<Region>* find_edge(RagEdge<Region>* edge);


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
  
  private:

    RagNode(Region node_int_) : node_int(node_int_), size(0) {}
    unsigned long long size;
    Region node_int;
    RagEdgeList edges;
    

};
 
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

template<typename Region> RagEdge<Region>* RagNode<Region>::find_edge(RagEdge<Region>* edge)
{
    typename RagEdgeList::iterator iter = std::find(edges.begin(), edges.end(), edge);
    RagEdge<Region>* actual_edge = 0;
    if (iter != edges.end()) {
        actual_edge = *iter;
    }
    return actual_edge;
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
