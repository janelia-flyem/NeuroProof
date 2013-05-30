#ifndef STACK_H
#define STACK_H

#include <boost/python.hpp>
#include "../FeatureManager/FeatureManager.h"
#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include "../Rag/Rag.h"
#include "AffinityPair.h"
#include "Glb.h"
#include <boost/tuple/tuple.hpp>
#include "../Algorithms/MergePriorityFunction.h"
#include "../Algorithms/MergePriorityQueue.h"

#include <time.h>

#include "../Algorithms/BatchMergeMRFh.h"
#include "../Utilities/unique_row_matrix.h"

#include "../Watershed/vigra_watershed.h"

#include "../Refactoring/RagUtils2.h"

#include <json/json.h>
#include <json/value.h>

namespace NeuroProof {
class LabelCount;
class Stack {
  public:
    Stack(Label* watershed_, int depth_, int height_, int width_, int padding_=1) : watershed(0), watershed2(0), feature_mgr(0), median_mode(false), gtruth(0), merge_mito(false)
    {
        rag = new Rag_uit();
        reinit_stack(watershed_, depth_, height_, width_, padding_);
    }
    
    Stack() : watershed(0), watershed2(0), feature_mgr(0), median_mode(false), depth(0), width(0), height(0), gtruth(0), merge_mito(false)
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

    void compute_synapse_volume(std::vector<Label>& seg_labels, std::vector<Label>& gt_labels);
    Label * get_label_volume();
    Label * get_label_volume_reverse();
    
    boost::python::tuple get_edge_loc2(RagEdge_uit* edge);
    void get_edge_loc(RagEdge_uit* edge, Label& x, Label& y, Label& z);

    void build_rag();
    void create_0bounds();
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
        return !(node->is_border());
    }
        
    Rag_uit* get_rag()
    {
        return rag;
    }


    double get_edge_weight(RagEdge_uit* edge)
    {
        return feature_mgr->get_prob(edge);
    }

#ifndef SETPYTHON
    void set_basic_features();
#endif

    void set_groundtruth(Label* pgt)
    {
        if (gtruth) {
            delete gtruth;   
        }
        gtruth = pgt;
    }
    int grab_max_overlap(Label seg_body, std::tr1::unordered_set<Label>& gt_orphans,
        Rag_uit* gt_rag, std::tr1::unordered_set<Label>& seg_matched,
        std::tr1::unordered_set<Label>& gt_matched);
    void compute_groundtruth_assignment();     			
    void compute_contingency_table();
    void compute_vi();
    void dump_vi_differences(double threshold);
    void modify_assignment_after_merge(Label node_keep, Label node_remove);
    void load_synapse_counts(std::tr1::unordered_map<Label, int>& synapse_bodies);
    bool is_excluded(Label node);
    void merge_nodes(Label node1, Label node2);
    void write_graph(string);
    int decide_edge_label(RagNode_uit* node1, RagNode_uit* node2);


    
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
    void set_exclusions(std::string synapse_json);
    void set_gt_exclusions();
    void set_body_exclusions(string exclusions_json);
    int remove_small_regions(unsigned long long threshold);
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

    ~Stack()
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

    void set_merge_mito(bool flag)
    {
        merge_mito = flag;
    }

  protected:
    void biconnected_dfs(std::vector<DFSStack>& dfs_stack);
    
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
    std::tr1::unordered_set<Label> exclusion_set;
   
    std::vector<std::vector<unsigned int> > all_locations;

    Label* gtruth; 	// both derived classes may use groundtruth for either learning or validation
    std::multimap<Label, Label>	assignment;
    std::multimap<Label, std::vector<LabelCount> > contingency;	
    std::multimap<double, Label> seg_overmerge_ranked;
    std::multimap<double, Label> gt_overmerge_ranked;

    double merge_vi;
    double split_vi;
    bool merge_mito;
};



class StackLearn: public Stack{

public:

    StackLearn(Label* watershed_, int depth_, int height_, int width_, int padding_=1): Stack(watershed_, depth_, height_, width_, padding_){}

    void learn_edge_classifier_lash(double, UniqueRowFeature_Label& , std::vector<int>&, const char* clfr_filename = NULL);
    void learn_edge_classifier_queue(double, UniqueRowFeature_Label& , std::vector<int>&, bool accumulate_all=false,  const char* clfr_filename = NULL);
    void learn_edge_classifier_flat(double, UniqueRowFeature_Label& , std::vector<int>&, const char* clfr_filename = NULL);
    void learn_edge_classifier_flat_subset(double, UniqueRowFeature_Label& , std::vector<int>&, const char* clfr_filename = NULL);

};

class StackPredict: public Stack{

public:

    StackPredict(Label* watershed_, int depth_, int height_, int width_, int padding_=1): Stack(watershed_, depth_, height_, width_, padding_){}
    
    void agglomerate_rag(double threshold, bool use_edge_weight, string output_path="", string classifier_path="", bool synapse_mode = false);
    void agglomerate_rag_queue(double threshold, bool use_edge_weight=false, string output_path="", string classifier_path="");     			
    void agglomerate_rag_flat(double threshold, bool use_edge_weight=false, string output_path="", string classifier_path="");
    void agglomerate_rag_mrf(double threshold, bool read_off, string output_path, string classifier_path);
    void agglomerate_rag_size(double threshold);

    void merge_mitochondria_a();
    void absorb_small_regions(double* prediction_vol, Label* label_vol);
    int absorb_small_regions2(double* prediction_vol, Label* label_vol, int threshold);

};

class LabelCount{
public:
      Label lbl;
      size_t count;
      LabelCount(): lbl(0), count(0) {};	 	 		
      LabelCount(Label plbl, size_t pcount): lbl(plbl), count(pcount) {};	 	 		
};

}

#endif
