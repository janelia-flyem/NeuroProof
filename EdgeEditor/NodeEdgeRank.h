#ifndef NODEEDGERANK_H
#define NODEEDGERANK_H

#include "EdgeRank.h"

#include <set>
#include <tr1/unordered_map>
#include <vector>

#define BIGBODY10NM 25000

namespace NeuroProof {

struct NodeRank {
    Node_uit id;
    unsigned long long size;
    bool operator<(const NodeRank& br2) const
    {
        if ((size > br2.size) || (size == br2.size && id < br2.id)) {
            return true;
        }
        return false;
    }
};

class NodeRankList {
  public:
    NodeRankList(Rag_uit* rag_) : rag(rag_), checkpoint(false) {}

    void insert(NodeRank item);
    bool empty();
    size_t size();
    NodeRank first();
    void pop();
    void remove(Node_uit id);
    void remove(NodeRank item);
    void start_checkpoint();
    void stop_checkpoint();
    void undo_one();
    void clear();
        
  private:

    struct HistoryElement {
        HistoryElement(NodeRank item_, bool inserted_node_) : item(item_), inserted_node(inserted_node_) {}
        NodeRank item;
        bool inserted_node;
    };

    Rag_uit* rag;
    std::set<NodeRank> node_list;
    std::tr1::unordered_map<Node_uit, NodeRank> stored_ids;
    bool checkpoint;
    std::vector<std::vector<HistoryElement> > history;
};

class NodeEdgeRank : public EdgeRank {
  public:
    NodeEdgeRank(Rag_uit* rag_) : EdgeRank(rag_), node_list(rag_) {}
    
    void examined_edge(NodePair node_pair, bool remove);
    void update_priority();
    bool get_top_edge(NodePair& top_edge_);
    void undo();
   
    bool is_finished()
    {
        return node_list.empty();
    }

    unsigned int get_num_remaining()
    {
        return node_list.size();
    }
    
    virtual ~NodeEdgeRank() {}

  protected:
    virtual RagNode_uit* find_most_uncertain_node(RagNode_uit* head_node) = 0;
    virtual void insert_node(Node_uit node) = 0;
    virtual void update_neighboring_nodes(Node_uit keep_node) = 0;

    double calc_voi_change(double size1, double size2,
        unsigned long long total_volume);
    
    NodeRankList node_list;

  private:
    NodePair top_edge;    
};

}

#endif
