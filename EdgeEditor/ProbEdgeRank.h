#ifndef PROBEDGERANK_H
#define PROBEDGERANK_H

#include "EdgeRank.h"
#include <map>

namespace NeuroProof {

class ProbEdgeRank : public EdgeRank {
  public:
    ProbEdgeRank(Rag_uit* rag_) : EdgeRank(rag_), lower(0.1), upper(0.1),
    start(0.1), ignore_size(1)  {}
   
    void initialize(double lower, double upper, double start,
            double ignore_size);
    void examined_edge(NodePair node_pair, bool remove); 
    void update_priority();
    bool get_top_edge(NodePair& top_edge_);
    void undo();
    bool is_finished(); 
    unsigned int get_num_remaining();

  private:    
    void grab_edge_ranking();
    
    typedef std::multimap<double, RagEdge_uit* > EdgeRanking;
    EdgeRanking edge_ranking;
    double lower, upper, start, ignore_size;

};

}

#endif

