/*!
 * Implements functionality for ranking edges for focused
 * examination by first ranking the most important nodes using
 * some criteria determined by inhertied classes.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef NODECENTRICRANK_H
#define NODECENTRICRANK_H

#include "EdgeRank.h"

#include <set>
#include <tr1/unordered_map>
#include <vector>

/*
 * Crude golden number that is used for a threshold size for
 * 'important' bodies.  This threshold should really not be
 * in actual implementations to be flexible for different type
 * of data and imaging resolutions.
*/
#define BIGBODY10NM 25000

namespace NeuroProof {

/*!
 * Fundamental element used by algorithms that rank edges
 * by examining the most important nodes.
*/
struct NodeRank {
    //! node id
    Node_t id;

    //! the importance of the node encoded in a long value
    unsigned long long size;

    /*!
     * Ranking of nodes determined by the size variable so
     * that a large size variable is ranked first.
     * \param br2 noderank element
     * \return true if lhs > rhs
    */
    bool operator<(const NodeRank& br2) const
    {
        if ((size > br2.size) || (size == br2.size && id < br2.id)) {
            return true;
        }
        return false;
    }
};

/*!
 * Datastructure for ranking node rank elements so that
 * the nodes with the largest 'size' come first.  This
 * implementation allows for checkpointing so that previous
 * states can be rolled back to if some action is undone.
 * Alternatively, after any undo, a recomputation of the entire
 * list could be performed.
*/
class NodeRankList {
  public:
    /*!
     * Constructor requires rag that the nodes belong to.
     * Checkpointing is initially disable.
     * \param rag_ pointer to RAG
    */
    NodeRankList(Rag_t* rag_) : rag(rag_), checkpoint(false) {}

    /*!
     * Insert node rank element into sorted list
     * \param item node rank element
    */
    void insert(NodeRank item);
    
    /*!
     * Determine whether the list is empty.
     * \return true if empty
    */
    bool empty();
    
    /*!
     * Determine the number of elements in the list.
     * \return number of elements
    */
    size_t size();
    
    /*!
     * Retrieve the first element in the list.
     * \return node rank element
    */
    NodeRank first();
    
    /*!
     * Remove first element in the list.
    */
    void pop();
    
    /*!
     * Remove element by searching for a node with
     * the given id.
     * \param id rag node id
    */
    void remove(Node_t id);
   
    /*!
     * Remove element by using a rank element.
     * \param item node rank element
    */ 
    void remove(NodeRank item);
    
    /*!
     * Set a rollback checkpoint for operations.
    */
    void start_checkpoint();
    
    /*!
     * Stop rollback checkpoint for operations.
    */
    void stop_checkpoint();
    
    /*!
     * Undo to last checkpoint.
    */
    void undo_one();
    
    /*!
     * Clear all datastructures within this class.
    */
    void clear();
        
  private:

    /*!
     * Structure for recording actions performed on the node list.
    */
    struct HistoryElement {
        HistoryElement(NodeRank item_, bool inserted_node_) : item(item_), inserted_node(inserted_node_) {}
        NodeRank item;
        bool inserted_node;
    };

    //! pointer to RAG
    Rag_t* rag;

    //! ordered set of node rank elements (first element has largest 'size')
    std::set<NodeRank> node_list;

    //! mapping between id and rank element to allow flexible, quick deletion
    std::tr1::unordered_map<Node_t, NodeRank> stored_ids;

    //! enables checkpoint mechanisms when true
    bool checkpoint;

    //! explains history of operations on this list
    std::vector<std::vector<HistoryElement> > history;
};

/*!
 * Abstract class defines interface and functionality for supporting
 * focused algorithms/strategies where the goal is the examine one node
 * at a time (using some priority order) and examining all of the edges
 * on that node that are most uncertain/important using some criteria
 * defined in the inherited classes.  The goal of ordering the editing
 * by node is to allow a manual editor to focus on one body at a time.
 * There is an additional advantage in that the node ranking can be relatively
 * simple and used as a way to restrict the potentially computationally
 * intensive algorithms for finding the most impactful edges.  Technically,
 * it is possible for a node that is already examined to require
 * re-examination after additional editing.
*/
class NodeCentricRank : public EdgeRank {
  public:
    /*!
     * Constructor that initializes the rag object and the node list.
     * \param rag_ pointer to RAG
    */
    NodeCentricRank(Rag_t* rag_) : EdgeRank(rag_), node_list(rag_) {}
   
    /*!
     * Handle whether the given edge (node pair) being examined
     * should be removed or not.
     * \param node_pair two nodes connected by edge
     * \param remove true for false edge
    */
    void examined_edge(NodePair node_pair, bool remove);
    
    /*!
     * Provide the edge with the most uncertainty as determined
     * by the underlying algorithm.
     * \param top_edge_ most uncertain/import node pair
     * \return true if a node pair is found
    */
    bool get_top_edge(NodePair& top_edge_);
    
    /*!
     * Reverse a previous decision made by examiend edge.
    */
    void undo();
   
    /*!
     * Determines if there are no more important/uncertain edges.
     * \return true if there are no more edges
    */
    bool is_finished()
    {
        return node_list.empty();
    }

    /*!
     * The number of remaining bodies in the list, not the estimated
     * number of remaining edges.  It is probably better to estimate
     * the number of remaining uncertain/important edges by making
     * decisions pseudo-randomly based on the underlying edge weight
     * to get a rough estimate and then undoing all of these decisions.
    */
    unsigned int get_num_remaining()
    {
        return node_list.size();
    }
   
    /*!
     * Virtual destructor for proper memory handling.
    */
    virtual ~NodeCentricRank() {}

  protected:
    /*!
     * Virtual function to be implemented by derived class.  The
     * function interface should return the current most uncertain
     * edge given the current most important node.
     * \param head_node pointer to rag node
     * \return pointer to node with an uncertaint connection to the head node
    */
    virtual RagNode_t* find_most_uncertain_node(RagNode_t* head_node) = 0;

    /*!
     * Virtual function to be implemented by derived class.  The
     * function takes the given node and determines whether it should be
     * added into the node rank list.
     * \param node id of rag node
    */
    virtual void insert_node(Node_t node) = 0;

    /*!
     * Virtual function to be implemented by derived class. The
     * function tries to determine whether, given a change to a certain
     * node (due to a merge with another node), other neighboring
     * nodes should be added and updated in the body rank list.
     * \param keep_node id of rag node that was just modified
    */
    virtual void update_neighboring_nodes(Node_t keep_node) = 0;

    /*!
     * Determine the change in information in the rag between
     * combining and not combining two nodes given a certain
     * size attribute.
     * \param size1 size of node1
     * \param size2 size of node2
     * \return change in information
    */
    double calc_voi_change(double size1, double size2,
        unsigned long long total_volume);
   
    //! ranking for nodes (nodes with the largest 'size' are first)
    NodeRankList node_list;

  protected:
    /*!
     * Updates the edge priority ranking.
    */
    void update_priority();

  private:
        
    //! represents the current most uncertain edge 
    NodePair top_edge;    
};

}

#endif
