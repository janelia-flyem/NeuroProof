/*!
 * Defines templated class for the edges used in the region adjacency
 * graph (RAG).  This class is templated because nodes are templated.
 * Despite this, unsigned int is primary node type supported by
 * functions in NeuroProof.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef RAGEDGE_H
#define RAGEDGE_H

// rag node functions called
#include "RagNode.h"

// used for finding hashes to index edge data
#include <boost/functional/hash.hpp>

namespace NeuroProof {

// forward declaration of ptr functors that will be friends of rag edge
template <typename Region>
class RagEdgePtrEq;

template <typename Region>
class RagEdgePtrCmp;

template <typename Region>
class RagEdgePtrHash;

/*!
 * Edge component in RAG.  The edge only stores a few properties by default.
 * Additional edge properties can be stored through the rag element interface.
 * Edge directionality is not handled by this class and must be added as a
 * property if desired.  Each edges only has two nodes.  Hyper-edges are not
 * supported.
*/
template <typename Region>
class RagEdge : public RagElement {
  public:
    /*!
     * Static function for creating rag edges.  All edges should be created
     * on the heap.
     * \param node1 node connected to new edge
     * \param node2 node connected to new edge
     * \return pointer to new rag edge
    */
    static RagEdge<Region>* New(RagNode<Region>* node1, RagNode<Region>* node2)
    {
        return new RagEdge(node1, node2);  
    }

    /*!
     * Static function for copying rag edges.  All edges should be created
     * on the heap.
     * \param edge edge to be copied
     * \return pointer to new rag edge
    */
    static RagEdge<Region>* New(const RagEdge<Region>& edge)
    {
        return new RagEdge(edge);  
    }

    /*!
     * Sets status of edge to true or false.  In NeuroProof, a false edge edge
     * generally exists as a constraint in the graph but does not actually indicate
     * a true adjacency.
     * \param false_edge_ false_edge flag
    */
    void set_false_edge(bool false_edge_);

    /*!
     * Sets preserve status of edge to true or false.  In NeuroProof, when preserve is true
     * the edge should be guarded from removal.
     * \param preserve_ preserve flag
    */
    void set_preserve(bool preserve_);

    /*!
     * Sets dirty status of edge to true or false.  Used in algorithms for marking
     * edges to be computed.
     * \param dirty_ dirty flag
    */
    void set_dirty(bool dirty_);

    /*!
     * Determines dirty status of edge
     * \return dirty status
    */
    bool is_dirty() const;

    /*!
     * Determines preserve status of edge
     * \return preserve status
    */
    bool is_preserve() const;

    /*!
     * Determines edge status
     * \return edge status
    */
    bool is_false_edge() const;

    /*!
     * Retrieves node 1 connected to edge
     * \return rag node
    */
    RagNode<Region>* get_node1() const;
    
    /*!
     * Retrieves node 2 connected to edge
     * \return rag node
    */
    RagNode<Region>* get_node2() const;
    
    /*!
     * Retrieve node connected to edge that is not the provided node
     * \param node rag node connected to edge
     * \return rag node
    */
    RagNode<Region>* get_other_node(RagNode<Region>* node) const;
    
    /*!
     * Gets the weight associated with the edge
     * \return weight value on edge
    */
    double get_weight() const;
    
    /*!
     * Sets weight for the edge
     * \param weight_ weight value for edge
    */
    void set_weight(double weight_);
    
   
    /*!
     * Replace nodes with specified nodes -- should not call in general
     * \param region1 rag node
     * \param region2 rag node
    */ 
    void set_edge(RagNode<Region>* region1, RagNode<Region>* region2);

    /*!
     * Get the size of the edge
     * \return size of edge
    */
    unsigned long long get_size() const;

    /*!
     * Set the size of the edge
     * \param size edge size
    */
    void set_size(unsigned long long size);

    /*!
     * Increase size of the edge
     * \param incr size of edge increment
    */
    void incr_size(unsigned long long incr = 1);

    /*!
     * Boolean comparison between edges based on node id order
     * \param edge2 rag edge
     * \return is less 
    */
    bool operator<(const RagEdge<Region>& edge2) const;

    /*!
     * Equivalence based on both node ids connected to edge
     * \param edge rag edge
     * \return is equivalent
    */
    bool operator==(const RagEdge<Region>& edge2) const;
    
    /*!
     * Creates hash based on node ids connected to edge -- used for indexing
     * \param rag edge
     * \return hash value
    */
    std::size_t operator()(const RagEdge<Region>& edge) const;

    // declare pointer functors friends
    friend class RagEdgePtrEq<Region>;
    friend class RagEdgePtrCmp<Region>;
    friend class RagEdgePtrHash<Region>;

  private:
    
    /*!
     * Private constructor to prevent stack allocation
     * \param node1 node connected to new edge
     * \param node2 node connected to new edge
    */
    RagEdge(RagNode<Region>* node1_, RagNode<Region>* node_);
    
    /*!
     * Noop: prevent assignment for edges
     * \param edge2 rag edge
     * \return updated rag edge
    */
    RagEdge<Region>&  operator=(const RagEdge<Region>& edge2) {}

    /*!
     * Private copy constructor to prevent stack allocation
     * \param edge2 rag edge to be copied
    */
    RagEdge(const RagEdge<Region>& edge2);

    /*!
     * Support function for overloaded equivalence operator
     * \param rag edge
     * \return is equivalent
    */
    bool is_equal(const RagEdge<Region>* edge2) const;

    /*!
     * Support function for overloaded less than operator
     * \param rag edge
     * \return is less 
    */
    bool is_less(const RagEdge<Region>* edge2) const;
    
    /*!
     * Support function for overloaded function operator for hash computation
     * \param rag edge
     * \param hash value
    */ 
    std::size_t compute_hash(const RagEdge<Region>* edge) const;

    //! node 1 for edge
    RagNode<Region>* node1;

    //! node 2 for edge
    RagNode<Region>* node2;

    //! weight associated with edge (default 0)
    double weight;

    //! size associated with edge (default 0)
    unsigned long long edge_size;

    //! preserve status flag
    bool preserve;

    //! false edge indication
    bool false_edge;

    //! dirty status flag
    bool dirty;
    
    // TODO: can add one more bool because of word alignment
};

// default Rag Edge type used for most of NeuroProof
typedef RagEdge<Node_t> RagEdge_t; 

// inlined functions
template<typename Region> inline RagEdge<Region>::RagEdge(RagNode<Region>* node1_, RagNode<Region>* node2_) :
    weight(0.0), edge_size(0), preserve(false), false_edge(false), dirty(false)
{
    // put the smaller node at node 1
    RagNodePtrCmp<Region> cmp;
    if (cmp(node1_, node2_)) { 
        node1 = node1_;
        node2 = node2_;
    } else {
        node1 = node2_;
        node2 = node1_;
    }
}

template<typename Region> inline void RagEdge<Region>::set_false_edge(bool false_edge_)
{
    false_edge = false_edge_;
}

template<typename Region> inline void RagEdge<Region>::set_preserve(bool preserve_)
{
    preserve = preserve_;
}

template<typename Region> inline void RagEdge<Region>::set_dirty(bool dirty_)
{
    dirty = dirty_;
}

template<typename Region> inline bool RagEdge<Region>::is_dirty() const
{
    return dirty;
}

template<typename Region> inline bool RagEdge<Region>::is_preserve() const
{
    return preserve;
}

template<typename Region> inline bool RagEdge<Region>::is_false_edge() const
{
    return false_edge;
}

template<typename Region> inline RagNode<Region>* RagEdge<Region>::get_node1() const
{
    return node1;
}

template<typename Region> inline RagNode<Region>* RagEdge<Region>::get_node2() const
{
    return node2;
}

template<typename Region> inline RagNode<Region>* RagEdge<Region>::get_other_node(RagNode<Region>* node) const
{
    if (node == node1) {
        return node2;
    } else {
        return node1;
    }
}

template<typename Region> inline double RagEdge<Region>::get_weight() const
{
    return weight;
}

template<typename Region> inline void RagEdge<Region>::set_weight(double weight_)
{
    weight = weight_;
}

template<typename Region> inline unsigned long long RagEdge<Region>::get_size() const
{
    return edge_size;
}

template<typename Region> inline void RagEdge<Region>::set_size(unsigned long long size)
{
    edge_size = size;
}

template<typename Region> inline void RagEdge<Region>::incr_size(unsigned long long incr)
{
    edge_size += incr;
}

template<typename Region> inline RagEdge<Region>::RagEdge(const RagEdge<Region>& edge2) :
    RagElement(edge2)
{
    node1 = edge2.node1; 
    node2 = edge2.node2;
    weight = edge2.weight;
    edge_size = edge2.edge_size;
    preserve = edge2.preserve;
    false_edge = edge2.false_edge;
    dirty = edge2.dirty;
}




template<typename Region> void RagEdge<Region>::set_edge(RagNode<Region>* node1_, RagNode<Region>* node2_) 
{
    // put the smaller node at node 1 as in constructor
    RagNodePtrCmp<Region> cmp;
    if (cmp(node1_, node2_)) { 
        node1 = node1_;
        node2 = node2_;
    } else {
        node1 = node2_;
        node2 = node1_;
    }
}

// support functions for overloaded operators and functors
template<typename Region> inline bool RagEdge<Region>::is_less(const RagEdge<Region>* edge2) const
{
    // order deterined by first node then second node
    if (node1 < edge2->node1) {
        return true;
    } else if ((node1)->get_node_id() == (edge2->node1)->get_node_id()
            && (node2)->get_node_id() < (edge2->node2)->get_node_id()) {
        return true;
    }
    return false;
}

template<typename Region> inline bool RagEdge<Region>::is_equal(const RagEdge<Region>* edge2) const
{
    return (((node1)->get_node_id() == (edge2->node1)->get_node_id())
            && ((node2)->get_node_id() == (edge2->node2)->get_node_id()));

}

template<typename Region> std::size_t RagEdge<Region>::compute_hash(const RagEdge<Region>* edge) const
{
    std::size_t seed = 0;
    
    // TODO: make more generic for other data types -- will only work well for
    // those data types <=32 bytes
    // NOTE: node id type must allow for casting to size_t
    boost::hash_combine(seed, std::size_t((edge->node1)->get_node_id()));
    boost::hash_combine(seed, std::size_t((edge->node2)->get_node_id()));
    return seed;    
}


// overloaded operators
template<typename Region> inline bool RagEdge<Region>::operator<(const RagEdge<Region>& edge2) const
{
    return is_less(&edge2);
}

template<typename Region> inline bool RagEdge<Region>::operator==(const RagEdge<Region>& edge2) const
{
    return is_equal(&edge2);
}

template<typename Region> inline std::size_t RagEdge<Region>::operator()(const RagEdge<Region>& edge) const
{
    return compute_hash(&edge);
}


/*!
 * Functor that determines whether edge pointers have the same edge
*/
template<typename Region>
struct RagEdgePtrEq {
    bool operator() (const RagEdge<Region>* edge1, const RagEdge<Region>* edge2) const
    {
        return edge1->is_equal(edge2);
    }
};

/*!
 * Functor that determines which edge pointer is less
*/
template<typename Region>
struct RagEdgePtrCmp {
    bool operator() (const RagEdge<Region>* edge1, const RagEdge<Region>* edge2) const
    {
        return edge1->is_less(edge2);
    }
};

/*!
 * Functor that determines hash for given edge pointer
*/
template<typename Region>
struct RagEdgePtrHash {
    std::size_t operator() (const RagEdge<Region>* edge2) const
    {
        return  edge2->compute_hash(edge2);
    }
};


}

#endif
