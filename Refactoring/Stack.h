#ifndef STACK_H
#define STACK_H

#include <boost/python.hpp>
#include "../FeatureManager/FeatureManager.h"
#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include "../Rag/Rag.h"
#include "../DataStructures/AffinityPair.h"
#include <boost/tuple/tuple.hpp>
#include "../Algorithms/MergePriorityFunction.h"
#include "../Algorithms/MergePriorityQueue.h"

#include <time.h>

#include "../Algorithms/BatchMergeMRFh.h"
#include "../Utilities/unique_row_matrix.h"

#include "../Watershed/vigra_watershed.h"

#include "../Refactoring/RagUtils2.h"
#include "../Rag/RagUtils.h"

#include <json/json.h>
#include <json/value.h>

typedef unsigned int Label;


namespace NeuroProof {
class Stack2 {
  public:
    Stack2(Label* watershed_, int depth_, int height_, int width_, int padding_=1) : watershed(0), watershed2(0), feature_mgr(0), median_mode(false), gtruth(0), merge_mito(false)
    {
        rag = new Rag_uit();
        reinit_stack(watershed_, depth_, height_, width_, padding_);
    }
    
    Stack2() : watershed(0), watershed2(0), feature_mgr(0), median_mode(false), depth(0), width(0), height(0), gtruth(0), merge_mito(false)
    {
        rag = new Rag_uit();
    }

    void reinit_stack(Label* watershed_, int depth_, int height_, int width_, int padding_)
    {
        if (!prediction_array.empty()) {
            for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                delete prediction_array[i];
            }
            prediction_array.clear();
        }
        if (watershed) {
            delete watershed;
        }
    
    
        watershed = watershed_;
        depth = depth_;
        height = height_;
        width = width_;
        padding = padding_;
    } 
    
    void reinit_watershed(Label * watershed_)
    {
        if (watershed) {
            delete watershed;
        }
        watershed = watershed_;
    }

    void reinit_stack2(Label* watershed_, int depth_, int height_, int width_, int padding_)
    {
        if (!prediction_array2.empty()) {
            for (unsigned int i = 0; i < prediction_array2.size(); ++i) {
                delete prediction_array2[i];
            }
            prediction_array2.clear();
        }
        if (watershed2) {
            delete watershed2;
        }
    
    
        watershed2 = watershed_;
        depth2 = depth_;
        height2 = height_;
        width2 = width_;
        padding2 = padding_;
    } 

    Label * get_label_volume();
    boost::python::tuple get_edge_loc2(RagEdge_uit* edge);
    void get_edge_loc(RagEdge_uit* edge, Label& x, Label& y, Label& z);

    void build_rag();
    void build_rag_border();
    void determine_edge_locations(bool use_probs=false);
    boost::python::list get_transformations();
    void disable_nonborder_edges();
    void enable_nonborder_edges();
    void init_mappings();

    void agglomerate_rag(double threshold);

    void add_prediction_channel(double * prediction_array_)
    {
        prediction_array.push_back(prediction_array_);
    
    }

    void add_empty_channel()
    {
        if (feature_mgr) {
            feature_mgr->add_channel();
        }
    }

    void add_prediction_channel2(double * prediction_array_)
    {
        prediction_array2.push_back(prediction_array_);
    }


    bool is_orphan(RagNode_uit* node)
    {
        return !(node->is_boundary());
    }
        
    Rag_uit* get_rag()
    {
        return rag;
    }


    double get_edge_weight(RagEdge_uit* edge)
    {
        return feature_mgr->get_prob(edge);
    }

    void set_groundtruth(Label* pgt)
    {
        if (gtruth) {
            delete gtruth;   
        }
        gtruth = pgt;
    }
    void merge_nodes(Label node1, Label node2);
    void write_graph(string);


    
    Label get_body_id(unsigned int x, unsigned int y, unsigned int z);
    Label get_gt_body_id(unsigned int x, unsigned int y, unsigned int z);

    bool add_edge_constraint(boost::python::tuple loc1, boost::python::tuple loc2);
    bool add_edge_constraint2(unsigned int x1, unsigned int y1, unsigned int z1,
        unsigned int x2, unsigned int y2, unsigned int z2);

    FeatureMgr * get_feature_mgr()
    {
        return feature_mgr;
    }
    void set_feature_mgr(FeatureMgr * feature_mgr_)
    {
        feature_mgr = feature_mgr_;
    }

    int get_num_bodies()
    {
        return rag->get_num_regions();        
    }

    int get_width() const
    {
        return width;
    }

    int get_height() const
    {
        return height;
    }

    int get_depth() const
    {
        return depth;
    }

    void write_graph_json(Json::Value& json_writer);
    int remove_inclusions();

    void reinit_rag()
    {
        delete rag;
        if (!watershed_to_body.empty()) {
            unsigned long long max_id = width * height * depth;
            for (unsigned long long i = 0; i < max_id; ++i) {
                watershed[i] = watershed_to_body[(watershed[i])];
            }
        }
        watershed_to_body.clear();
        rag = new Rag_uit();
    }

    ~Stack2()
    {
        delete rag;
        if (!prediction_array.empty()) {
            for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                delete [] prediction_array[i];
            }
        }

        delete [] watershed;
        if (feature_mgr) {
            delete feature_mgr;
        }
    }
    struct DFSStack {
        Label previous;
        RagNode_uit* rag_node;  
        int count;
        int start_pos;
    };

  protected:
    void biconnected_dfs(std::vector<DFSStack>& dfs_stack);
    void load_synapse_counts(std::tr1::unordered_map<Label, int>& synapse_bodies);
    
    Rag_uit * rag;
    Label* watershed;
    Label* watershed2;
    std::vector<double*> prediction_array;
    std::vector<double*> prediction_array2;
    std::tr1::unordered_map<Label, Label> watershed_to_body; 
    std::tr1::unordered_map<Label, std::vector<Label> > merge_history; 
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;
    typedef std::tr1::unordered_map<RagEdge_uit*, double> EdgeCount; 
    typedef std::tr1::unordered_map<RagEdge_uit*, Location> EdgeLoc; 
    typedef std::tr1::unordered_set<RagEdge_uit*, RagEdgePtrHash<Node_uit>, RagEdgePtrEq<Node_uit> >  EdgeHash;
    
    int depth, height, width;
    int padding;
    int depth2, height2, width2;
    int padding2;

    std::tr1::unordered_set<Label> visited;
    std::tr1::unordered_map<Label, int> node_depth;
    std::tr1::unordered_map<Label, int> low_count;
    std::tr1::unordered_map<Label, Label> prev_id;
    std::vector<std::vector<OrderedPair> > biconnected_components; 
    
    std::vector<OrderedPair> stack;
   
    EdgeCount best_edge_z;
    EdgeLoc best_edge_loc;

    FeatureMgr * feature_mgr;
    bool median_mode;
    EdgeHash border_edges;
   
    std::vector<std::vector<unsigned int> > all_locations;

    Label* gtruth; 	// both derived classes may use groundtruth for either learning or validation
    std::multimap<Label, Label>	assignment;
    std::multimap<double, Label> seg_overmerge_ranked;
    std::multimap<double, Label> gt_overmerge_ranked;

    double merge_vi;
    double split_vi;
    bool merge_mito;
};




}

#endif
