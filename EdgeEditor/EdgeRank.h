/*!
 * This interface in this file is used by all focused proofreading
 * algorithms to be compatible with the edge editor functions.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef EDGERANK_H
#define EDGERANK_H

#include "../Rag/Rag.h"
#include <boost/tuple/tuple.hpp>
#include <string>

namespace NeuroProof {

//! define an edge as a pair of node ids
typedef boost::tuple<Node_t, Node_t> NodePair;

/*!
 * EdgeRank is an abstract base class that defines the interface
 * that is called by the edge editor.  The interface defines
 * functionality for accessing edges in a RAG that are considered
 * the most important or morst uncertain and are therefore
 * worth examination.
*/
class EdgeRank {
  public:
    /*!
     * Constructor that initializes the rag object.
     * \param rag_ pointer to RAG
    */
    EdgeRank(Rag_t* rag_) : rag(rag_), num_processed(0) {}
   
    /*!
     * Handle whether the given edge (node pair) being examined
     * should be removed or not.
     * \param node_pair two nodes connected by edge
     * \param remove true for false edge
    */
    virtual void examined_edge(NodePair node_pair, bool remove) = 0; 

    /*!
     * Provide the edge with the most uncertainty as determined
     * by the underlying algorithm.
     * \param top_edge_ most uncertain/import node pair
     * \return true if a node pair is found
    */
    virtual bool get_top_edge(NodePair& top_edge_) = 0;

    virtual void set_label(int label_){};
    /*!
     * Reverse a previous decision made by examiend edge.
    */
    virtual void undo() = 0;
    
    /*!
     * Determines if there are no more important/uncertain edges.
     * \return true if there are no more edges
    */
    virtual bool is_finished() = 0; 
    
    /*!
     * The number of remaining important edges.  In some cases, this number
     * might not be known and will be replaced with some other proxy.
     * It is probably better to estimate the number of remaining
     * uncertain/important edges by making decisions pseudo-randomly
     * based on the underlying edge weight to get a rough estimate and
     * then undoing all of these decisions.
    */
    virtual unsigned int get_num_remaining() = 0;
    
    /*!
     * Determine the name of a given derived class.
     * \return the name of class
    */
    virtual std::string get_identifier() = 0;

    /*!
     * Determine the number of decisions (examined_edge calls) made.
     * \return number of edge decisions made
    */
    unsigned int get_num_processed()
    {
        return num_processed;
    }
    /*!
     * Virtual destructor for proper memory handling.
    */
    virtual ~EdgeRank() {}
    
    virtual void save_labeled_edges(std::string& save_fname){};

  protected:
    /*!
     * Uppdates the edge priority ranking.
    */
    virtual void update_priority() = 0;

    //! pointer to RAG generally assumed to have edges with weights
    Rag_t* rag;

    //! number of edge decisions (minus undos) performed
    int num_processed;
};


}

#endif
