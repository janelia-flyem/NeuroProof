#ifndef EDGEEDITOR_H
#define EDGEEDITOR_H

#include "../Rag/RagNodeCombineAlg.h"
#include "../Rag/Rag.h"
#include "NodeSizeRank.h"
#include "ProbEdgeRank.h"
#include "OrphanRank.h"
#include "SynapseRank.h"

#include <vector>
#include <json/value.h>
#include <set>
#include <map>
#include <boost/tuple/tuple.hpp>

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

class EdgeEditor {
  public:
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

    EdgeEditor(Rag_uit& rag_, double min_val_, double max_val_,
            double start_val_, Json::Value& json_vals); 
    void export_json(Json::Value& json_writer);

    // depth currently not set
    void set_body_mode(double ignore_size_, int depth);
    
    // synapse orphan currently not used
    void set_orphan_mode(double ignore_size_);
   
    // synapse orphan currently not used
    void set_edge_mode(double lower, double upper, double start, double ignore_size_ = 1);

    void set_synapse_mode(double ignore_size_);

 
    void setEdge(NodePair node_pair, double weight);
    unsigned int getNumRemaining();
    void removeEdge(NodePair node_pair, bool remove);
    
    std::vector<Node_uit> getQAViolators(unsigned int threshold);
    void estimateWork();

    NodePair getTopEdge(Location& location);
    bool undo();
  
    bool isFinished();

    void set_custom_mode(EdgeRank* edge_mode_);

    ~EdgeEditor();

  private:
    typedef std::tr1::unordered_map<std::string, boost::shared_ptr<Property> >
        NodePropertyMap; 

    void removeEdge2(NodePair node_pair, bool remove,
            std::vector<std::string>& node_properties);

    void reinitialize_scheduler();

    bool undo2();

    void clear_history();

    Rag_uit& rag;

    struct EdgeHistory {
        Node_uit node1;
        Node_uit node2;
        unsigned long long size1;
        unsigned long long size2;
        unsigned long long bound_size1;
        unsigned long long bound_size2;
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

    std::vector<EdgeHistory> history_queue;
    LowWeightCombine join_alg;

    double min_val, max_val, start_val; 
    
    unsigned int num_processed;    
    unsigned int num_syn_processed;    
    unsigned int num_body_processed;    
    unsigned int num_edge_processed;    
    unsigned int num_orphan_processed;    
    unsigned int num_slices;
    unsigned int current_depth;
    unsigned int num_est_remaining;    
    
    double ignore_size;
    unsigned long long volume_size;

    std::vector<std::string> node_properties;

    const std::string SynapseStr;

    // ordering algorithms supported by builtin
    // import utility -- this can be extended easily
    // other rank algorithms can always be used
    // generically but its stats and mode will
    // not be exportable by the edge editor
    ProbEdgeRank* prob_edge_mode;
    OrphanRank* orphan_edge_mode;
    SynapseRank* synapse_edge_mode;
    NodeSizeRank* body_edge_mode;

    EdgeRank* edge_mode;
};


}

#endif
