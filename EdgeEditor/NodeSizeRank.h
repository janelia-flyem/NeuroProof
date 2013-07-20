/*!
 * Implements functionality for ranking edges by using
 * the size of the nodes combined with the uncertainty of
 * the edges.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef NODESIZERANK_H
#define NODESIZERANK_H

#include "NodeCentricRank.h"

#include <string>

namespace NeuroProof {

/*!
 * Concrete implementation of type node centric rank.  This
 * algorithm will examine nodes from largest to smallest and
 * then examine paths between that node and other, high-affinity
 * large nodes.
*/
class NodeSizeRank : public NodeCentricRank {
  public:
    /*!
     * Constructor that initializes the rag object, sets a
     * default threshold for examining a body, and the default
     * maximum path length (0 is unbounded).
     * \param rag_ pointer to RAG
    */
    NodeSizeRank(Rag_t* rag_) : NodeCentricRank(rag_), ignore_size(BIGBODY10NM),
            voi_change_thres(0.0), depth(0), volume_size(0) {}
  
    /*!
     * Initialize (or reinitialize) the body rank list by adding
     * all nodes larger than a threshold.  TODO: current the ignore
     * size threhsold is used with a hard-coded value that is supposed
     * to represent the smallest 'real' body possible.  This should
     * vary as the application dictates.
     * \param ignore_size size below which nodes are ignored
     * \param depth maximum path length in affinity analysis (0 is unbounded)
    */
    void initialize(double ignore_size, unsigned int depth);

    /*!
     * Determine the name of a given derived class.
     * \return the name of class
    */
    std::string get_identifier()
    {
        return std::string("nodesize");
    }
  
    /*!
     * The function interface should return the current most uncertain
     * edge given the current most important node.  The most uncertain
     * edge is the one with the highest information change in node
     * size.  This function is called by the NodeCentricRank class.
     * \param head_node pointer to rag node
     * \return pointer to node with an uncertaint connection to the head node
    */ 
    RagNode_t* find_most_uncertain_node(RagNode_t* head_node);

    /*!
     * The function takes the given node and adds it into the node rank
     * list.  This function is called by the NodeCentricRank class.
     * \param node id of rag node
    */
    void insert_node(Node_t node);

    /*!
     * The function tries to determine whether, given a change to a
     * certain node (due to a merge with another node), other neighboring
     * nodes should be added and updated in the body rank list.  This
     * function is called by the NodeCentricRank class.
     * \param keep_node id of rag node that was just modified
    */
    void update_neighboring_nodes(Node_t keep_node);
  
  private:
    //! size below which nodes are not examined
    double ignore_size;

    //! threshold for information change above which the path is examined
    double voi_change_thres;

    //! path length limit between a pair of nodes (0 is unbounded)
    unsigned int depth;

    //! size of all of the nodes in the RAG
    unsigned long long volume_size;
};

}

#endif
