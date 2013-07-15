#ifndef LOCALEDGEPRIORITY_H
#define LOCALEDGEPRIORITY_H

#include "../Rag/RagNodeCombineAlg.h"
#include "../Rag/Rag.h"
#include "../DataStructures/AffinityPair.h"

#include <boost/tuple/tuple.hpp>
#include <vector>
#include <json/value.h>
#include <set>
#include <queue>
#include <map>

#define BIGBODY10NM 25000

namespace NeuroProof {

class LowWeightCombine : public RagNodeCombineAlg {
  public:
    void post_edge_move(RagEdge<unsigned int>* edge_new,
            RagEdge<unsigned int>* edge_remove)
    {
        double weight = edge_remove->get_weight();
        edge_new->set_weight(weight);
    }

    void post_edge_join(RagEdge<unsigned int>* edge_keep,
            RagEdge<unsigned int>* edge_remove)
    {
        double weight = edge_remove->get_weight();
        if ((weight <= edge_keep->get_weight() && (edge_keep->get_weight() <= 1.0))
                || (weight > 1.0) ) { 
            edge_keep->set_weight(weight);
        }
    }

    void post_node_join(RagNode<unsigned int>* node_keep,
            RagNode<unsigned int>* node_remove) {}
};

//TODO: create modes that will work one body at a time

class BodyRankList;

class LocalEdgePriority {
  public:
    typedef boost::tuple<Node_uit, Node_uit> NodePair;
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

    LocalEdgePriority(Rag_uit& rag_, double min_val_, double max_val_,
            double start_val_, Json::Value& json_vals); 
    void export_json(Json::Value& json_writer);

    // depth currently not set
    void set_body_mode(double ignore_size_, int depth);
    
    // synapse orphan currently not used
    void set_orphan_mode(double ignore_size_, double threshold, bool synapse_orphan);
   
    // synapse orphan currently not used
    void set_edge_mode(double lower, double upper, double start);

    void set_synapse_mode(double ignore_size_);

 
    void setEdge(NodePair node_pair, double weight);
    unsigned int getNumRemaining();
    void removeEdge(NodePair node_pair, bool remove);
    
    std::vector<Node_uit> getQAViolators(unsigned int threshold);
    
    void estimateWork();

    NodePair getTopEdge(Location& location);
    void updatePriority();
    bool isFinished(); 
    bool undo();
    double find_path(RagNode_uit* rag_node_head, RagNode_uit* rag_node_dest);
    
    typedef std::multimap<double, RagEdge_uit* > EdgeRanking;

  private:
    typedef std::tr1::unordered_map<std::string, boost::shared_ptr<Property> >
        NodePropertyMap; 

    void removeEdge2(NodePair node_pair, bool remove,
            std::vector<std::string>& node_properties);

    void grabAffinityPairs(RagNode_uit* rag_node_head, int path_restriction,
            double connection_threshold, bool preserve);

    void reinitialize_scheduler();

    double voi_change(double size1, double size2, unsigned long long total_volume);

    bool undo2();

    void clear_history();

    Rag_uit& rag;

    struct EdgeHistory {
        Node_uit node1;
        Node_uit node2;
        unsigned long long size1;
        unsigned long long size2;
        std::vector<Node_uit> node_list1;
        std::vector<Node_uit> node_list2;
        NodePropertyMap node_properties1;
        NodePropertyMap node_properties2;
        std::vector<double> edge_weight1;
        std::vector<double> edge_weight2;
        std::vector<bool> preserve_edge1;
        std::vector<bool> preserve_edge2;
        std::vector<bool> false_edge1;
        std::vector<bool> false_edge2;
        std::vector<std::vector<boost::shared_ptr<Property> > > property_list1;
        std::vector<std::vector<boost::shared_ptr<Property> > > property_list2;
        bool remove;
        double weight;
        bool false_edge;
        bool preserve_edge;
        std::vector<PropertyPtr> property_list_curr;
    };

    struct BestNode {
        RagNode_uit* rag_node_curr;
        RagEdge_uit* rag_edge_curr;
        double weight;
        int path;
        Node_uit second_node;
    };
    struct BestNodeCmp {
        bool operator()(const BestNode& q1, const BestNode& q2) const
        {
            return (q1.weight < q2.weight);
        }
    };

    std::vector<EdgeHistory> history_queue;
    std::vector<std::string> property_names;
    LowWeightCombine join_alg;
    EdgeRanking edge_ranking;

    // assume that 0 body will never be added as a screen
    EdgeRanking rag_grab_edge_ranking(Rag_uit& rag, double min_val,
            double max_val, double start_val, double ignore_size=27,
            Node_uit screen_id = 0);

    double min_val, max_val, start_val; 
    
    double curr_prob;

    unsigned int num_processed;    
    unsigned int num_syn_processed;    
    unsigned int num_body_processed;    
    unsigned int num_edge_processed;    
    unsigned int num_orphan_processed;    
    unsigned int num_slices;
    unsigned int current_depth;
    unsigned int num_est_remaining;    
    
    double last_prob;
    int last_decision;

    bool prune_small_edges;
    bool orphan_mode;
    bool synapse_mode;
    bool prob_mode;
    double ignore_size;
    double ignore_size_orig;
    BodyRankList * body_list;
    int approx_neurite_size;
    unsigned long long volume_size;

    std::vector<std::string> node_properties;

    typedef AffinityPair::Hash AffinityPairsLocal;
    AffinityPairsLocal affinity_pairs;

    typedef std::priority_queue<BestNode, std::vector<BestNode>, BestNodeCmp> BestNodeQueue;
    BestNode best_node_head;
    BestNodeQueue best_node_queue; 
    unsigned int debug_count;

    Json::Value json_body_edges;
    Json::Value json_synapse_edges;
    Json::Value json_orphan_edges;

    std::vector<OrderedPair> debug_queue;
    std::vector<RagEdge_uit* > body_edges;
    std::vector<RagEdge_uit* > synapse_edges;
    std::vector<RagEdge_uit* > orphan_edges;

    const std::string OrphanStr;
    const std::string ExaminedStr;
    const std::string SynapseStr;
};


}

#endif
