#ifndef STACKCONTROLLER_H
#define STACKCONTROLLER_H

#include "Stack.h"
#include <map>

#include <json/json.h>
#include <json/value.h>

namespace NeuroProof {

class RagNodeCombineAlg;

class StackController {
  public:
    StackController(Stack* stack_) : stack(stack_), updated(false) {}

    Stack* get_stack()
    {
        return stack;
    }

    void build_rag();
    int remove_inclusions();
    void merge_labels(Label_t label_remove, Label_t label_keep,
            RagNodeCombineAlg* combine_alg, bool ignore_rag = false);

    int absorb_small_regions(VolumeProbPtr boundary_pred, int threshold,
                    std::tr1::unordered_set<Label_t>& exclusions);
    int remove_small_regions(int threshold,
                    std::tr1::unordered_set<Label_t>& exclusions);

    int match_regions_overlap(Label_t label, std::tr1::unordered_set<Label_t>& candidate_regions,
        RagPtr gt_rag, std::tr1::unordered_set<Label_t>& labels_matched,
        std::tr1::unordered_set<Label_t>& gtlabels_matched);
    void set_body_exclusions(string exclusions_json);
    void dilate_labelvol(int disc_size);
    void dilate_gt_labelvol(int disc_size);

    void compute_vi(double& merge, double& split);    
    void compute_vi(double& merge, double& split, std::multimap<double, Label_t>& label_ranked,
            std::multimap<double, Label_t>& gt_ranked);

    bool is_true_edge(Label_t label1, Label_t label2);

    unsigned int get_num_labels()
    {
        RagPtr rag = stack->get_rag();
        if (!rag) {
            throw ErrMsg("No rag defined for stack");
        }

        return rag->get_num_regions();
    }

    unsigned int get_xsize()
    {
        VolumeLabelPtr labelvol = stack->get_labelvol();
        if (!labelvol) {
            throw ErrMsg("No label volume defined for stack"); 
        }
    
        return labelvol->shape(0);
    }

    unsigned int get_ysize()
    {
        VolumeLabelPtr labelvol = stack->get_labelvol();
        if (!labelvol) {
            throw ErrMsg("No label volume defined for stack"); 
        }
    
        return labelvol->shape(1);
    }

    unsigned int get_zsize()
    {
        VolumeLabelPtr labelvol = stack->get_labelvol();
        if (!labelvol) {
            throw ErrMsg("No label volume defined for stack"); 
        }

        return labelvol->shape(2);
    }

    void serialize_stack(const char* h5_name, const char* graph_name,
            bool optimal_prob_edge_loc);
    
    virtual void serialize_graph_info(Json::Value& json_write) {}
    int find_edge_label(Label_t label1, Label_t label2);
    void compute_groundtruth_assignment();

  private:
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;
    typedef std::tr1::unordered_map<RagEdge_uit*, double> EdgeCount; 
    typedef std::tr1::unordered_map<RagEdge_uit*, Location> EdgeLoc; 
    
    struct LabelCount{
        Label_t lbl;
        size_t count;
        LabelCount(): lbl(0), count(0) {};	 	 		
        LabelCount(Label_t plbl, size_t pcount): lbl(plbl), count(pcount) {};	 	 		
    };
    

    void update_assignment(Label_t label_remove, Label_t label_keep);
    VolumeLabelPtr dilate_label_edges(VolumeLabelPtr ptr, int disc_size);
    VolumeLabelPtr generate_boundary(VolumeLabelPtr ptr);   
    void compute_contingency_table();
    void determine_edge_locations(EdgeCount& best_edge_z,
        EdgeLoc& best_edge_loc, bool use_probs);

    void serialize_graph(const char* graph_name, bool optimal_prob_edge_loc);
    void serialize_labels(const char* h5_name);
 
    Stack* stack;
    bool updated;
    std::tr1::unordered_map<Label_t, std::vector<LabelCount> > contingency;	
    std::tr1::unordered_map<Label_t, Label_t> assignment;
};

}

#endif
