/*!
 * Interface and definition of the node element that is a fundamental
 * unit of the Region Adjancency Graph (RAG).  While this class and the
 * corresponding Rag are templated to encourage genericity, in general
 * unsigned int will be the dominant node type supported in NeuroProof 
 * 
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/


#ifndef RAGNODE_H
#define RAGNODE_H

#include "RagElement.h"
#include "../Utilities/Glb.h"
#include <vector>
#include <algorithm>
#include <map>
#include <set>

// macro for property of boundary-size type
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
 * Inherits property interface from RagElement.  To reduce the memory profile
 * and make the implementation generic, only a few node attributes are
 * stored by default.  The Region type can be any type that implements the
 * necessary operator overloads and conversion function.
*/ 
template <typename Region>
class RagNode : public RagElement {
  public:

    /*!
     * Static function for creating rag nodes.  All edges should be created
     * on the heap.
     * \param node_int node unique identifier
     * \return pointer to new rag node
    */
    static RagNode<Region>* New(Region node_int)
    {
        return new RagNode(node_int);
    }

    /*!
     * Static function for copying rag nodes.  All nodess should be created
     * on the heap.
     * \param node node to be copied
     * \return pointer to new rag node 
    */
    static RagNode<Region>* New(const RagNode<Region>& node)
    {
        return new RagNode(node);  
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
    
    /*!
     * Convenience function that sets size of boundary size property
     * \param size_ boundary size
    */
    void set_boundary_size(unsigned long long size_);

    /*!
     * Reset the Region element/id associated with the node
     * \param region a region type
    */ 
    void set_node_id(Region region);

    /*!
     * Increase the size of the node by certain increment
     * \param incr size of the increment
    */ 
    void incr_size(unsigned long long incr = 1);

    /*!
     * Convenience function that increments size of boundary size property
     * \param incr size of boundary increment
    */ 
    void incr_boundary_size(unsigned long long incr = 1);

    /*!
     * Convenience function that retrieves the boundary size property
     * \return boundary size
    */
    unsigned long long get_boundary_size();

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
   
    /*!
     * Returns the lengths of the border plus boundary around the node
     * \return border size
    */
    unsigned long long compute_border_length();
 	
    /*!
     * Boolean comparison between nodes based on node id order
     * \param node2 rag node
     * \return is less 
    */
    bool operator<(const RagNode<Region>& node2) const;
    
    /*!
     * Equivalence based on node ids
     * \param node2 rag node
     * \return is equivalent
    */
    bool operator==(const RagNode<Region>& node2) const;

    /*!
     * Creates hash based on node id -- used for indexing
     * \param rag node
     * \return hash value
    */
    size_t operator()(const RagNode<Region>& node) const;

    //! Defines list of edges 
    typedef std::vector<RagEdge<Region>* > RagEdgeList;

    //! Defines iterator type for edge list
    typedef typename RagEdgeList::iterator edge_iterator;

    /*!
     * Iterator class for traversing nodes (not including itself)
     * connected to node
    */
    class node_iterator {
      public:
          typedef node_iterator self_type;
          typedef RagNode<Region>* value_type;
          typedef RagNode<Region>* reference;
          typedef RagNode<Region>** pointer;
          typedef std::forward_iterator_tag iterator_category;
          typedef int difference_type;

        /*!
         * Default constructor of iterator type
        */
        node_iterator();
        
        /*!
         * Constructor baed on rag edge list iterator and main rag node
         * \param rag_node_ node whose neighbors will be traversed
         * \param ragnode_iter_ iterator for edge list
        */
        node_iterator(RagNode<Region>* rag_node_, typename RagEdgeList::iterator ragnode_iter_) : rag_node(rag_node_), ragnode_iter(ragnode_iter_) {}
        
        /*!
         * Copy constructor for iterator
         * \param iterator2 node iterator
        */
        node_iterator(const node_iterator& iterator2) : ragnode_iter(iterator2.ragnode_iter), rag_node(iterator2.rag_node) {}
        
        /*!
         * Overloaded assignment operator
         * \param iterator2 node iterator
        */
        node_iterator& operator=(const node_iterator& iterator2)
        {
            ragnode_iter = iterator2.ragnode_iter;
            rag_node = iterator2.rag_node;
            return *this;
        }
        
        /*!
         * Overloaded pre increment
         * \return iterator
        */
        node_iterator& operator++()
        {
            ragnode_iter++;
            return *this;
        }
        
        /*!
         * Overloaded post increment
         * \return iterator
        */
        node_iterator operator++(int junk)
        {
            node_iterator temp = *this;
            ragnode_iter++;
            return temp;
        }

        /*!
         * Dereferencing operation for iterator gives node pointer
         * \return rag node pointer
        */
        RagNode<Region>* operator*()
        { 
            RagNode<Region> * temp =  (*ragnode_iter)->get_node1();
            if (temp != rag_node) {
                return temp;
            } else {
                return ((*ragnode_iter)->get_node2());
            }
        }
        
        /*!
         * Pointer reference for iterator gives pointer to node pointer
         * \return pointer to rag node pointer
        */
        RagNode<Region>** operator->()
        {
            RagNode<Region> * temp =  (*ragnode_iter)->get_node1();
            if (temp != rag_node) {
                return &temp;
            } else {
                return &((*ragnode_iter)->get_node2());
            }
        }
        
        /*!
         * Check equivalence between two iterators
         * \param iterator2 node iterator
         * \return is equivalent
        */
        bool operator==(const node_iterator& iterator2) const
        {
            return (ragnode_iter == iterator2.ragnode_iter);
        }
        
        /*!
         * Check if iterators are not equivalent
         * \param iterator2 node iterator
         * \return is not equivalent
        */
        bool operator!=(const node_iterator& iterator2) const
        {
            return (ragnode_iter != iterator2.ragnode_iter);
        }

      private:

        //! wrap edge list iterator for traversing
        typename RagEdgeList::iterator ragnode_iter;

        //! main node whose partners are traversed by iterator
        RagNode<Region>* rag_node;
    };

    /*!
     * Determines the starting edge iterator
     * \return edge iterator
    */
    edge_iterator edge_begin();

    /*!
     * Determines the ending edge iterator
     * \return edge iterator
    */
    edge_iterator edge_end();

    /*!
     * Determines the starting node iterator
     * \return node iterator
    */
    node_iterator node_begin();

    /*!
     * Determines the ending node iterator
     * \return node iterator
    */
    node_iterator node_end();

    /*!
     * Convencience function for accessing boundary size property and
     * determining whether the node is on the boundary or not
     * \return is boundary
    */ 
    bool is_boundary();

  private:

    /*!
     * Private constructor to prevent stack allocation
     * \param node_int_ node unique identifier
    */
    RagNode(Region node_int_);
   
    /*!
     * Noop: prevent assignment of nodes
     * \param node2 rag node
     * \return updated rag node
    */ 
    RagNode<Region>&  operator=(const RagNode<Region>& node2) {}
   
    /*!
     * Private copy constructor to prevent stack allocation
     * \param edge2 rag edge to be copied
    */
    RagNode(const RagNode<Region>& node2);

    //! vector of edges connected to the node
    RagEdgeList edges;

    //! size of the node (could be a property but is not for convenience)
    unsigned long long size;

    /*!
     * Unique identifier for node (usually unsigned int in NeuroProof).
     * If this is a user defined type it needs to overload several operators
     * as well as the size_t type cast.  This unique identifier could be
     * automatically created and any other identifier could be a property.
     * This strategy leads to more flexibility for the user but it does
     * allow for nodes with the same id to be created that are not supposed
     * to be equal.
    */
    Region node_int;
};

// default node type used for most of NeuroProof by default
typedef Index_t Node_t;

// default rag node type used for most of NeuroProof by default
typedef RagNode<Node_t> RagNode_t; 

template<typename Region> unsigned long long RagNode<Region>::compute_border_length()  {
    unsigned long long count = 0;
    // computer borders from edges
    for (RagNode<Region>::edge_iterator iter = this->edge_begin(); iter != this->edge_end(); ++iter) {
        if ((*iter)->is_false_edge()) {
            continue;
        }
        count += (*iter)->get_size();
    }
    // adds boundary size
    count += (get_boundary_size());

    return count;

}

//inline functions implementations
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

template<typename Region> inline void RagNode<Region>::set_boundary_size(unsigned long long size_)
{
    set_property(BOUNDARY_SIZE, size_);
}

template<typename Region> inline void RagNode<Region>::set_node_id(Region region)
{
    node_int = region;
}

template<typename Region> inline void RagNode<Region>::incr_size(unsigned long long incr)
{
    size += incr;
}

template<typename Region> inline void RagNode<Region>::incr_boundary_size(unsigned long long incr)
{
    unsigned long long boundary_size = 
        get_property<unsigned long long>(BOUNDARY_SIZE);
    set_property(BOUNDARY_SIZE, (boundary_size + incr));
}

template<typename Region> inline unsigned long long RagNode<Region>::get_boundary_size()
{
    return get_property<unsigned long long>(BOUNDARY_SIZE);
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

template<typename Region> inline typename RagNode<Region>::edge_iterator RagNode<Region>::edge_begin()
{
    return edge_iterator(edges.begin());
}

template<typename Region> inline typename RagNode<Region>::edge_iterator RagNode<Region>::edge_end()
{
    return edge_iterator(edges.end());
}

template<typename Region> inline typename RagNode<Region>::node_iterator RagNode<Region>::node_begin()
{
    return node_iterator(this, edges.begin());
}

template<typename Region> inline typename RagNode<Region>::node_iterator RagNode<Region>::node_end()
{
    return node_iterator(this, edges.end());
}

template<typename Region> inline bool RagNode<Region>::is_boundary()
{
    return (get_property<unsigned long long>(BOUNDARY_SIZE) != 0);
}

template<typename Region> inline RagNode<Region>::RagNode(Region node_int_) :
    size(0), node_int(node_int_)
{
    // sets boundary size property as a convenience
    set_property(BOUNDARY_SIZE, (unsigned long long)(0));
}

template<typename Region> inline RagNode<Region>::RagNode(const RagNode<Region>& node2) : 
    RagElement(node2)
{
    edges = node2.edges;
    size = node2.size;
    node_int = node2.node_int;
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
    // TODO: make more generic for >32 bytes
    // NOTE: node id type must allow for casting to size_t 
    return node.node_int; 
}

/*!
 * Functor that determines whether node pointers have the same node
*/
template<typename Region>
struct RagNodePtrEq {
    bool operator() (const RagNode<Region>* node1, const RagNode<Region>* node2) const
    {
        return (node1->get_node_id() == node2->get_node_id());
    }
};


/*!
 * Functor that determines which node pointer is less
*/
template<typename Region>
struct RagNodePtrCmp {
    bool operator() (const RagNode<Region>* node1, const RagNode<Region>* node2) const
    {
        return (node1->get_node_id() < node2->get_node_id());
    }

};

/*!
 * Functor that determines hash for given node pointer
*/
template<typename Region>
struct RagNodePtrHash {
    size_t operator() (const RagNode<Region>* node) const
    {
        // NOTE: node id type must allow for casting to size_t 
        return  node->get_node_id();
    }
};

}

#endif
