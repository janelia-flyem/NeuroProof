/*!
 * Implements functionality for ranking edges by using
 * the number of synapse in a node combined with the uncertainty of
 * the edges.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef SYNAPSERANK_H
#define SYNAPSERANK_H

#include "NodeCentricRank.h"

#include <string>

namespace NeuroProof {

/*!
 * Concrete implementation of type node centric rank.  This
 * algorithm will examine nodes from largest to smallest and
 * then examine paths between that node and other, high-affinity
 * nodes with synapse annotations.
*/
class SynapseRank : public NodeCentricRank {
  public:
    /*!
     * Constructor that initializes the rag object and sets a
     * default threshold for examining a body (in this case it
     * is a fraction of a synapse).
     * \param rag_ pointer to RAG
    */
    SynapseRank(Rag_t* rag_) : NodeCentricRank(rag_), ignore_size(0.1),
            voi_change_thres(0.0), volume_size(0),
            SynapseStr("synapse_weight") {}
  
    /*!
     * Initialize (or reinitialize) the body rank list by adding
     * all nodes with a synapse annotation.
     * \param ignore_size threshold used to determine importance of a node
     * \param depth maximum path length in affinity analysis (0 is unbounded)
    */
    void initialize(double ignore_size, double upper);
    
    /*!
     * Determine the name of a given derived class.
     * \return the name of class
    */
    std::string get_identifier()
    {
        return std::string("synapse");
    }

    /*!
     * The function interface should return the current most uncertain
     * edge given the current most important node.  The most uncertain
     * edge is the one with the highest information change in synapse
     * assignment.  This function is called by the NodeCentricRank class.
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
    //! threshold used to determine whether a node is important
    double ignore_size;
    
    //! probability threshold
    double connection_limit;
    
    //! threshold for information change above which the path is examined
    double voi_change_thres;
   
    //! number of synapse annotations in the entire RAG
    unsigned long long volume_size;
    
    //! access string for synapse node property
    const std::string SynapseStr;
};

}

#endif
