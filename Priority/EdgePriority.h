#ifndef EDGEPRIORITY_H
#define EDGEPRIORITY_H

#include "../DataStructures/Rag.h"
#include <vector>
#include <boost/tuple/tuple.hpp>

namespace NeuroProof {

template <typename Region>
class EdgePriority {
  public:
    typedef boost::tuple<Region, Region> NodePair;
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

    EdgePriority(Rag<Region>& rag_) : rag(rag_), updated(false) {}
    virtual NodePair getTopEdge(Location& location) = 0;
    virtual bool isFinished() = 0;
    virtual void updatePriority() = 0;
    // low weight == no connection
    virtual void setEdge(NodePair node_pair, double weight);

  protected:
    void setUpdated(bool status);
    bool isUpdated(); 
    Rag<Region>& rag;
    
  private:
    bool updated;
};


template <typename Region> void EdgePriority<Region>::setUpdated(bool status)
{
    updated = status;
}

template <typename Region> bool EdgePriority<Region>::isUpdated()
{
    return updated;
}

template <typename Region> void EdgePriority<Region>::setEdge(NodePair node_pair, double weight)
{
    RagEdge<Region>* edge = rag.find_rag_edge(boost::get<0>(node_pair), boost::get<1>(node_pair));
    edge->set_weight(weight);
}

}

#endif

