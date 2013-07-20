/*!
 * Implements a focused editing algorithm based on
 * the probability of edges.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef PROBEDGERANK_H
#define PROBEDGERANK_H

#include "EdgeRank.h"
#include <map>

namespace NeuroProof {

/*!
 * ProbEdgeRank inherits from EdgeRank and determines which
 * edge is the most important.  In this implementation,
 * edges are prioritized that have a low weight (a high chance
 * of being a false edge).  The size of the connecting bodies
 * is not considered once the body is larger than the threshold.
*/
class ProbEdgeRank : public EdgeRank {
  public:
    /*! 
     * Constructor that establishes the default bounds that
     * restrict the edges that are examined.
     * \param rag pointer to RAG
    */
    ProbEdgeRank(Rag_t* rag_) : EdgeRank(rag_), lower(0.1), upper(0.1),
        start(0.1), ignore_size(1)  {}
  
    /*!
     * Interface for initializing or reinitializing the algorithm
     * to allow for examining uncertain/important edges.
     * \param lower lower bound for edge weight
     * \param upper upper bound for edge weight
     * \param start starting weight for examination
     * \param ignore_size threshold for ignoring edges
    */
    void initialize(double lower, double upper, double start,
            double ignore_size);
    
    /*!
     * Handle whether the given edge (node pair) being examined
     * should be removed or not.
     * \param node_pair two nodes connected by edge
     * \param remove true for false edge
    */
    void examined_edge(NodePair node_pair, bool remove); 
    
    /*!
     * Provide the edge with weight closest to the starting
     * weight for examination.
     * \param top_edge_ most uncertain/import node pair
     * \return true if a node pair is found
    */
    bool get_top_edge(NodePair& top_edge_);
    
    /*!
     * Reverse a previous decision made by examiend edge.
    */
    void undo();
    
    /*!
     * Reverse a previous decision made by examiend edge.
    */
    bool is_finished(); 
    
    /*!
     * The number of edges currently within the lower and upper
     * uncertainty range.  It is probably better to estimate
     * the number of remaining uncertain/important edges by making
     * decisions pseudo-randomly based on the underlying edge weight
     * to get a rough estimate and then undoing all of these decisions.
    */
    unsigned int get_num_remaining();

    /*!
     * Determine the name of a given derived class.
     * \return the name of class
    */
    std::string get_identifier()
    {
        return std::string("probedge");
    }

  private:    
    /*!
     * Updates the edge priority ranking.
    */
    void update_priority();
    
    /*!
     * Finds all the edges within the provided lower
     * and upper bound.  The edges are ranked by their
     * closeness to the starting weight for examination
    */ 
    void grab_edge_ranking();
   
    //! defines structure for ranking edges by weight
    typedef std::multimap<double, RagEdge_t* > EdgeRanking;

    //! edge ranking used to prioritize uncertain/important edges
    EdgeRanking edge_ranking;

    //! lower, upper uncertainty bounds; starting weight for examination
    double lower, upper, start;
        
    //! the threshold size for considering nodes
    double ignore_size;
};

}

#endif

