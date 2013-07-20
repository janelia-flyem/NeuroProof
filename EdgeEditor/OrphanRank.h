/*!
 * Implements functionality for ranking edges by considering
 * the path that most likely connects a non-boundary node to
 * a node touching a boundary.
 * 
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef ORPHANRANK_H
#define ORPHANRANK_H

#include "NodeCentricRank.h"

#include <string>

namespace NeuroProof {

/*!
 * Concrete implementation of type node centric rank.  This
 * algorithm is working under the assumption that graph nodes
 * that are not touching are incorrect.  (In the case of a neuronal
 * reconstruction, this is a cell that does not exit the image stack.  While
 * it is possible to have a large dataset where entire cells
 * are contained within the volume, this will not be the case
 * in smaller datasets.  The cell will exit at least one of the faces
 * of the stack.)  The non-boundary node is called an orphan.
 * This algorithm finds the best potential paths for connection
 * this orphan to a non-orphan node.
*/  
class OrphanRank : public NodeCentricRank {
  public:
    /*!
     * Constructor that initializes the rag object, sets a
     * default threshold for examining a body.
     * \param rag_ pointer to RAG
    */
    OrphanRank(Rag_t* rag_) : NodeCentricRank(rag_),
        SynapseStr("synapse_weight"), ignore_size(BIGBODY10NM) {}
  
    /*!
     * Initialize (or reinitialize) the body rank list by adding
     * all orphan nodes that contain either a synapse annotation
     * or are larger than the threshold provided.
     * \param ignore_size size below which nodes are ignored
    */
    void initialize(double ignore_size);

    /*!
     * Determine the name of a given derived class.
     * \return the name of class
    */
    std::string get_identifier()
    {
        return std::string("orphan");
    }

    /*!
     * The function interface should return the current most uncertain
     * edge given the current most important node.  The most uncertain
     * edge is the one that most likely connects to a non-orphan body.
     * This function is called by the NodeCentricRank class.
     * \param head_node pointer to rag node
     * \return pointer to node with an uncertaint connection to the head node
    */
    RagNode_t* find_most_uncertain_node(RagNode_t* head_node);
    
    /*!
     * The function takes the given node and adds it into the node rank
     * list if it is still an orphan.  This function is called by the
     * NodeCentricRank class.
     * \param node id of rag node
    */
    void insert_node(Node_t node);
    
    /*!
     * The function tries to determine whether, given a change to a
     * certain node (due to a merge with another node), other neighboring
     * nodes should be added and updated in the body rank list since they
     * may now have potential paths to not being orphans.  This
     * function is called by the NodeCentricRank class.
     * \param keep_node id of rag node that was just modified
    */
    void update_neighboring_nodes(Node_t keep_node);
  
  private:
    //! access string for synapse node property
    const std::string SynapseStr;

    //! size below which nodes are not examined (except if they have a synapse)
    double ignore_size;
};

}

#endif
