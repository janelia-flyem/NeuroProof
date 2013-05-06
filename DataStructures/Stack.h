#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include "Rag.h"
#include "AffinityPair.h"
#include "../FeatureManager/FeatureManager.h"
#include "Glb.h"
#include <boost/python.hpp>
#include <boost/tuple/tuple.hpp>
#include "../Algorithms/MergePriorityFunction.h"
#include "../Algorithms/MergePriorityQueue.h"
#include <time.h>

#include "../Algorithms/BatchMergeMRFh.h"
#include "../Utilities/unique_row_matrix.h"

#include "../Watershed/vigra_watershed.h"

#include "../Algorithms/RagAlgs.h"

#ifndef STACK_H
#define STACK_H

namespace NeuroProof {
class LabelCount;
class Stack {
  public:
    Stack(Label* watershed_, int depth_, int height_, int width_, int padding_=1) : watershed(0), watershed2(0), feature_mgr(0), median_mode(false), gtruth(0)
    {
        rag = new Rag<Label>();
        reinit_stack(watershed_, depth_, height_, width_, padding_);
    }
    
    Stack() : watershed(0), watershed2(0), feature_mgr(0), median_mode(false), depth(0), width(0), height(0), gtruth(0)
    {
        rag = new Rag<Label>();
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
    
    boost::python::tuple get_edge_loc(RagEdge<Label>* edge);
    void get_edge_loc(RagEdge<Label>* edge, Label& x, Label& y, Label& z);

    void build_rag();
    void build_rag_border();
    void determine_edge_locations();
    boost::python::list get_transformations();
    void disable_nonborder_edges();
    void enable_nonborder_edges();

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


    bool is_orphan(RagNode<Label>* node)
    {
        return !(node->is_border());
    }
        
    Rag<Label>* get_rag()
    {
        return rag;
    }


    double get_edge_weight(RagEdge<Label>* edge)
    {
        return feature_mgr->get_prob(edge);
    }

    void set_basic_features();

    void set_groundtruth(Label* pgt) {gtruth = pgt; }
    void compute_groundtruth_assignment();     			
    void compute_contingency_table();
    void compute_vi();
    void modify_assignment_after_merge(Label node_keep, Label node_remove);
    void write_graph(string);
    int decide_edge_label(RagNode<Label>* node1, RagNode<Label>* node2);


    
    Label get_body_id(unsigned int x, unsigned int y, unsigned int z);

    bool add_edge_constraint(boost::python::tuple loc1, boost::python::tuple loc2);

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

    int remove_inclusions();

    ~Stack()
    {
        delete rag;
        if (!prediction_array.empty()) {
            for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                delete prediction_array[i];
            }
        }

        delete watershed;
        if (feature_mgr) {
            delete feature_mgr;
        }
    }
    struct DFSStack {
        Label previous;
        RagNode<Label>* rag_node;  
        int count;
        int start_pos;
    };

  protected:
    void biconnected_dfs(std::vector<DFSStack>& dfs_stack);
    
    Rag<Label> * rag;
    Label* watershed;
    Label* watershed2;
    std::vector<double*> prediction_array;
    std::vector<double*> prediction_array2;
    std::tr1::unordered_map<Label, Label> watershed_to_body; 
    std::tr1::unordered_map<Label, std::vector<Label> > merge_history; 
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;
    typedef std::tr1::unordered_map<RagEdge<Label>*, unsigned long long> EdgeCount; 
    typedef std::tr1::unordered_map<RagEdge<Label>*, Location> EdgeLoc; 
    typedef std::tr1::unordered_set<RagEdge<Label>*, RagEdgePtrHash<Label>, RagEdgePtrEq<Label> >  EdgeHash;
    
    int depth, height, width;
    int padding;
    int depth2, height2, width2;
    int padding2;

    boost::shared_ptr<PropertyList<Label> > node_properties_holder;
    std::tr1::unordered_set<Label> visited;
    std::tr1::unordered_map<Label, int> node_depth;
    std::tr1::unordered_map<Label, int> low_count;
    std::tr1::unordered_map<Label, Label> prev_id;
    std::vector<std::vector<OrderedPair<Label> > > biconnected_components; 
    
    std::vector<OrderedPair<Label> > stack;
   
    EdgeCount best_edge_z;
    EdgeLoc best_edge_loc;

    FeatureMgr * feature_mgr;
    bool median_mode;
    EdgeHash border_edges;
    
    Label* gtruth; 	// both derived classes may use groundtruth for either learning or validation
    std::multimap<Label, Label>	assignment;
    std::multimap<Label, std::vector<LabelCount> > contingency;	
};


void Stack::build_rag_border()
{
    if (feature_mgr && (feature_mgr->get_num_features() == 0)) {
        feature_mgr->add_median_feature();
        median_mode = true; 
    } 
    
    std::vector<double> predictions(prediction_array.size(), 0.0);
    std::vector<double> predictions2(prediction_array.size(), 0.0);

    unsigned int plane_size = width * height;
    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                unsigned int spot0 = watershed[curr_spot];
                unsigned int spot1 = watershed2[curr_spot];

                if (!spot0 || !spot1) {
                    continue;
                }

                assert(spot0 != spot1);

                RagNode<Label> * node = rag->find_rag_node(spot0);
                if (!node) {
                    node = rag->insert_rag_node(spot0);
                }

                RagNode<Label> * node2 = rag->find_rag_node(spot1);

                if (!node2) {
                    node2 = rag->insert_rag_node(spot1);
                }

                for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                    predictions[i] = prediction_array[i][curr_spot];
                }
                for (unsigned int i = 0; i < prediction_array2.size(); ++i) {
                    predictions2[i] = prediction_array2[i][curr_spot];
                }

                if (feature_mgr && !median_mode) {
                    feature_mgr->add_val(predictions, node);
                    feature_mgr->add_val(predictions2, node2);
                }

                rag_add_edge(rag, spot0, spot1, predictions, feature_mgr);
                rag_add_edge(rag, spot0, spot1, predictions2, feature_mgr);

                node->incr_border_size();
                node2->incr_border_size();

                border_edges.insert(rag->find_rag_edge(node, node2));
            }
        }
    }

    watershed_to_body[0] = 0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label id = (*iter)->get_node_id();
        watershed_to_body[id] = id;
    }
}

void Stack::disable_nonborder_edges()
{
    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if (border_edges.find(*iter) == border_edges.end()) {
            (*iter)->set_false_edge(true);
        }
    }
}

void Stack::enable_nonborder_edges()
{
    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if (!((*iter)->is_preserve())) {
            (*iter)->set_false_edge(false);
        }
    }
}

void Stack::agglomerate_rag(double threshold)
{
    if (threshold == 0.0) {
        return;
    }

    MergePriority* priority = new ProbPriority(feature_mgr, rag);
    priority->initialize_priority(threshold);


    while (!(priority->empty())) {

        RagEdge<Label>* rag_edge = priority->get_top_edge();

        if (!rag_edge) {
            continue;
        }

        RagNode<Label>* rag_node1 = rag_edge->get_node1();
        RagNode<Label>* rag_node2 = rag_edge->get_node2();
        Label node1 = rag_node1->get_node_id(); 
        Label node2 = rag_node2->get_node_id(); 
        rag_merge_edge_median(*rag, rag_edge, rag_node1, priority, feature_mgr);
        watershed_to_body[node2] = node1;
        for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
            watershed_to_body[*iter] = node1;
        }
        merge_history[node1].push_back(node2);
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);
    }

    EdgeHash border_edges2 = border_edges;
    border_edges.clear();
    for (EdgeHash::iterator iter = border_edges2.begin(); iter != border_edges2.end(); ++iter) {
        Label body1 = watershed_to_body[(*iter)->get_node1()->get_node_id()];
        Label body2 = watershed_to_body[(*iter)->get_node2()->get_node_id()];

        if (body1 != body2) {
            border_edges.insert(rag->find_rag_edge(body1, body2)); 
        
            /*
            RagNode<Label>* n1 = rag->find_rag_node(body1);
            RagNode<Label>* n2 = rag->find_rag_node(body2);
            // print info on edges
            RagEdge<Label>* temp_edge = rag->find_rag_edge(body1, body2);
            unsigned long long edge_size = temp_edge->get_size();
            unsigned long long total_edge_size1 = 0;
            unsigned long long total_edge_size2 = 0;

            for (RagNode<Label>::edge_iterator eiter = n1->edge_begin(); eiter != n1->edge_end(); ++eiter) {
                if (!((*eiter)->is_preserve() || (*eiter)->is_false_edge())) {
                    total_edge_size1 += (*eiter)->get_size();
                }
            }
            for (RagNode<Label>::edge_iterator eiter = n2->edge_begin(); eiter != n2->edge_end(); ++eiter) {
                if (!((*eiter)->is_preserve() || (*eiter)->is_false_edge())) {
                    total_edge_size2 += (*eiter)->get_size();
                }
            }
            double prob1 = edge_size / double(total_edge_size1);
            double prob2 = edge_size / double(total_edge_size2);
            std::cout << "Body 1: " << body1 << " Body 2: " << body2 << " Size: " << edge_size << " overlap: " << prob1 << " " << prob2 << std::endl;
            */
        }
    }
}

boost::python::list Stack::get_transformations()
{
    boost::python::list transforms;

    for (std::tr1::unordered_map<Label, Label>::iterator iter = watershed_to_body.begin();
           iter != watershed_to_body.end(); ++iter)
    {
        if (iter->first != iter->second) {
            //boost::python::tuple<Label, Label> mapping(iter->first, iter->second);
            transforms.append(boost::python::make_tuple(iter->first, iter->second));
        }
    } 

    return transforms;
}

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
    
    void agglomerate_rag(double threshold, bool use_edge_weight, string output_path="", string classifier_path="");
    void agglomerate_rag_queue(double threshold, bool use_edge_weight=false, string output_path="", string classifier_path="");     			
    void agglomerate_rag_flat(double threshold, bool use_edge_weight=false, string output_path="", string classifier_path="");
    void agglomerate_rag_mrf(double threshold, bool read_off, string output_path, string classifier_path);
    void agglomerate_rag_size(double threshold);

    void merge_mitochondria_a();
    void absorb_small_regions(double* prediction_vol, Label* label_vol);
    void absorb_small_regions2(double* prediction_vol, Label* label_vol);

};

class LabelCount{
public:
      Label lbl;
      size_t count;
      LabelCount(): lbl(0), count(0) {};	 	 		
      LabelCount(Label plbl, size_t pcount): lbl(plbl), count(pcount) {};	 	 		
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
