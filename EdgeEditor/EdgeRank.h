#ifndef EDGERANK_H
#define EDGERANK_H

#include "../Rag/Rag.h"
#include <boost/tuple/tuple.hpp>

namespace NeuroProof {

typedef boost::tuple<Node_uit, Node_uit> NodePair;

class EdgeRank {
  public:
    EdgeRank(Rag_uit* rag_) : rag(rag_), num_processed(0) {}
   
    virtual void examined_edge(NodePair node_pair, bool remove) = 0; 
    virtual void update_priority() = 0;
    virtual bool get_top_edge(NodePair& top_edge_) = 0;
    virtual void undo() = 0;
    virtual bool is_finished() = 0; 
    virtual unsigned int get_num_remaining() = 0;

    unsigned int get_num_processed()
    {
        return num_processed;
    }
    virtual ~EdgeRank() {}

  protected:
    Rag_uit* rag;
    int num_processed;
};

}

#endif
