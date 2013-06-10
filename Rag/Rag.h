/*!
 * Main interface to Region Adjacency Graph (RAG).  The Rag is templated
 * because the rag nodes allow for templated index type.  Despite this flexibility,
 * downstream NeuroProof code generates implements functionality assuming unsigned
 * int.  The Rag is a holder of edges and nodes and is the preferred interface for
 * creating new edges and nodes.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/ 

#ifndef RAG_H
#define RAG_H

#include "RagEdge.h"
#include "RagNode.h"
#include "../Utilities/ErrMsg.h"

// has set used for efficient accessing of edges and nodes
#include <tr1/unordered_set>

#include <boost/shared_ptr.hpp>

namespace NeuroProof {

/*!
 * Templated class for RAG.  Defines interface for creating rag elements
 * and provides some functionality for traversing these elements.  All
 * nodes stored will have a unique node identifier associated with it.
 * One cannot insert multiple edges are nodes with the same unique identifiers.
*/
template <typename Region>
class Rag {
  public:
    
    /*!
     * Rag constructor
    */
    Rag();
    
    /*!
     * Rag copy constructor that copies rag edge and rag node data
     * to new heap memory
     * \param dup_rag rag to be copied
    */ 
    Rag(const Rag<Region>& dup_rag);
    
    /*!
     * Assignment operator for Rag (invokes copy constructor)
     * \param dup_rag to be assigned from
    */
    Rag& operator=(const Rag<Region>& dup_rag);
    
    /*!
     * Destroys all memory associated with nodes and edges
    */
    ~Rag();

    /*!
     * Returns a rag node or 0 based on a unique node identifier
     * \param region unique node identifier
     * \return pointer to matching rag node
    */
    RagNode<Region>* find_rag_node(Region region);
    
    /*!
     * Makes a new rag node on the heap given the unique node identifier
     * \return pointer to new rag node
    */
    RagNode<Region>* insert_rag_node(Region region);
  
    /*!
     * Find rag edge given two unique node identifiers believed to have
     * an edge between them (0 if no edge exists)
     * \param region1 unique identifier
     * \param region2 unique identifier
     * \return pointer to matching rag edge
    */
    RagEdge<Region>* find_rag_edge(Region region1, Region region2);
    
    /*!
     * Find rag edge given pointers to rag nodes that have an edge between
     * them (0 if no edge exists)
     * \param node1 pointer to rag node
     * \param node2 pointer to rag node
     * \return pointer to matching rag edge
    */
    RagEdge<Region>* find_rag_edge(RagNode<Region>* node1, RagNode<Region>* node2);
    
    /*!
     * Makes a new rag edge on the heap between two previously created rag nodes
     * \param rag_node1 pointer to rag node
     * \param rag_node2 pointer to rag node
     * \return pointer to newly created rag edge
    */
    RagEdge<Region>* insert_rag_edge(RagNode<Region>* rag_node1, RagNode<Region>* rag_node2);

    /*!
     * Removes rag node from the rag and deletes it from the heap
     * \param rag_node pointer to rag node to be removed
    */
    void remove_rag_node(RagNode<Region>* rag_node);
    
    /*!
     * Removes rag edge from the rag and deletes it from the heap
     * \param rag_edge pointer to rag edge to be removed
    */
    void remove_rag_edge(RagEdge<Region>* rag_edge);

    /*!
     * Retrieves the number of nodes stored in the rag
     * \return number of nodes
    */
    size_t get_num_regions() const;

    /*!
     * Retrieves the number of edges stored in the rag
     * \return number of edges
    */
    size_t get_num_edges() const;

    /*!
     * Retrieve the cumulative size of all the nodes in the graph
     * \return size of all nodes
    */
    unsigned long long get_rag_size();

    //! Container for all edges using a hash
    typedef std::tr1::unordered_set<RagEdge<Region>*, RagEdgePtrHash<Region>, RagEdgePtrEq<Region> >  EdgeHash;
    
    //! Container for all nodes using a hash
    typedef std::tr1::unordered_set<RagNode<Region>*, RagNodePtrHash<Region>, RagNodePtrEq<Region> >  NodeHash;

    //! Defines iterator type for nodes container
    typedef typename NodeHash::iterator nodes_iterator;
    
    //! Defines iterator type for edges container
    typedef typename EdgeHash::iterator edges_iterator;


    /*!
     * Determines the starting iterator for the nodes container
     * \return nodes iterator
    */
    nodes_iterator nodes_begin();

    /*!
     * Determines the starting iterator for the edges container
     * \return edges iterator
    */
    edges_iterator edges_begin();
   
    /*!
     * Determines the ending iterator for the nodes container
     * \return nodes iterator
    */ 
    nodes_iterator nodes_end();
    
    /*!
     * Determines the ending iterator for the edges container
     * \return edges iterator
    */
    edges_iterator edges_end();

  private:
    
    /*!
     * Swaps variables between current objet and another rag
     * \param rag_core rag to be copied
    */
    void swap_em(Rag<Region>* rag_core);
    
    /*!
     * Creates rag nodes and edge used for quickly probing
     * rag containers
    */
    void init_probes();
    
    /*!
     * Retrieve a rag node given another rag node pointer or 0 if no
     * such rag node exists in the rag (two different rag node pointers
     * are equivalent if they have the same unique identifier)
     * \param node pointer to rag node
     * \return pointer to rag node in node list 
    */
    RagNode<Region>* find_rag_node(RagNode<Region>* node);
 
    /*!
     * Retrieve a rag edge given another rag edge pointer or 0 if no
     * such rag edge exists in the rag (two different rag edge pointers
     * are equivalent if they have the same unique identifier)
     * \param edge pointer to rag edge 
     * \return pointer to rag edge in edge list 
    */
    RagEdge<Region>* find_rag_edge(RagEdge<Region>* edge);

    /*!
     * Creates an edge for probing containers given rag node pointers
     * (existing probe edge is redefined to avoid new heap allocation)
     * \param rag_node1 pointer to rag node
     * \param rag_node2 pointer to rag node
     * \return pointer to created rag edge
    */
    RagEdge<Region>* get_probe_edge(RagNode<Region>* rag_node1, RagNode<Region>* rag_node2);

    /*!
     * Creates rag edge and corresponding rag nodes given two unique
     * node identifiers (existing probe edge and nodes are redefined
     * to avoid new heap allocations)
     * \param region1 unique node identifier
     * \param region2 unique node identifier
    */
    RagEdge<Region>* get_probe_edge(Region region1, Region region2);

    //! hash container for all unique edges
    EdgeHash rag_edges;

    //! hash container for all unique nodes
    NodeHash rag_nodes;

    //! heap created rag edge reused for each query to probe edge container
    RagEdge<Region>* probe_rag_edge;

    //! heap created rag node reused for each query to probe node container
    RagNode<Region>* probe_rag_node;

    //! heap created rag node reused for each query to probe node container
    RagNode<Region>* probe_rag_node2;
};

// unsigned int Rag type used in primarily in downstream NeuroProof
typedef Rag<Node_uit> Rag_uit;
typedef boost::shared_ptr<Rag_uit> RagPtr;


template <typename Region> Rag<Region>::Rag()
{
    init_probes();    
} 

template <typename Region> Rag<Region>::Rag(const Rag<Region>& dup_rag)
{
    // create new probes on the heap
    init_probes();
    
    // create new nodes on the heap copying the previous node data and their properties 
    for (typename EdgeHash::const_iterator iter = dup_rag.rag_edges.begin(); iter != dup_rag.rag_edges.end(); ++iter) {
        RagEdge<Region>* rag_edge = RagEdge<Region>::New(**iter);
        rag_edges.insert(rag_edge);
    }
    
    // create new edges on the heap copying the previous edge data and their properties 
    for (typename NodeHash::const_iterator iter = dup_rag.rag_nodes.begin(); iter != dup_rag.rag_nodes.end(); ++iter) {
        RagNode<Region>* rag_node = RagNode<Region>::New(**iter);
        rag_nodes.insert(rag_node);
    }

    // link new nodes to new edges (not the old copies)
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

    // link new edges to new nodes (not the old copies)
    for (typename NodeHash::const_iterator iter = dup_rag.rag_nodes.begin(); iter != dup_rag.rag_nodes.end(); ++iter) {
        RagNode<Region> * scan_node = const_cast<RagNode<Region>*>(*iter);
        RagNode<Region> * curr_node = find_rag_node(scan_node);
    
        for (typename RagNode<Region>::edge_iterator edge_iter = scan_node->edge_begin(); edge_iter != scan_node->edge_end(); ++edge_iter) {
            curr_node->remove_edge(*edge_iter);
            curr_node->insert_edge(find_rag_edge(*edge_iter));
        }
    }
}

template <typename Region> Rag<Region>& Rag<Region>::operator=(const Rag<Region>& dup_rag)
{
    // copy and swap
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

// inlined functions
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
   
    // prevent duplication of edges in rag 
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

    for (unsigned int i = 0; i < edge_list.size(); ++i) {
        rag_edges.erase(edge_list[i]);
        edge_list[i]->get_node1()->remove_edge(edge_list[i]); 
        edge_list[i]->get_node2()->remove_edge(edge_list[i]); 
        delete edge_list[i];
    }

    rag_nodes.erase(rag_node);
    delete rag_node;
}

template <typename Region> inline void Rag<Region>::remove_rag_edge(RagEdge<Region>* rag_edge)
{
    typename EdgeHash::iterator rag_edge_iter = rag_edges.find(rag_edge);
    if (rag_edge_iter == rag_edges.end()) {
        throw ErrMsg("edge does not exist");
    }

    rag_edges.erase(rag_edge);
    rag_edge->get_node1()->remove_edge(rag_edge);
    rag_edge->get_node2()->remove_edge(rag_edge);

    delete rag_edge;
}

template <typename Region> inline size_t Rag<Region>::get_num_regions() const
{
    return rag_nodes.size();
}

template <typename Region> inline size_t Rag<Region>::get_num_edges() const
{
    return rag_edges.size();
} 

template <typename Region> inline typename Rag<Region>::nodes_iterator Rag<Region>::nodes_begin()
{
    return nodes_iterator(rag_nodes.begin());
}

template <typename Region> inline typename Rag<Region>::edges_iterator Rag<Region>::edges_begin()
{
    return edges_iterator(rag_edges.begin());
}

template <typename Region> inline typename Rag<Region>::nodes_iterator Rag<Region>::nodes_end()
{
    return nodes_iterator(rag_nodes.end());
}

template <typename Region> inline typename Rag<Region>::edges_iterator Rag<Region>::edges_end()
{
    return edges_iterator(rag_edges.end());
}


template <typename Region> void Rag<Region>::swap_em(Rag<Region>* rag_core)
{
    std::swap(rag_edges, rag_core->rag_edges);
    std::swap(rag_nodes, rag_core->rag_nodes);
    std::swap(probe_rag_edge, rag_core->probe_rag_edge);
    std::swap(probe_rag_node, rag_core->probe_rag_node);
    std::swap(probe_rag_node2, rag_core->probe_rag_node2);
}

template <typename Region> unsigned long long Rag<Region>::get_rag_size()
{
    unsigned long long total_size = 0;
    for (typename Rag<Region>::nodes_iterator iter = nodes_begin();
            iter != nodes_end(); ++iter) {
        total_size += (*iter)->get_size();
    }
    return total_size;
}

}

#endif
