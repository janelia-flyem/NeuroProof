#ifndef STACKCONTROLLER_H
#define STACKCONTROLLER_H

#include "Stack.h"
#include <map>

namespace NeuroProof {

class RagNodeCombineAlg;

class StackController {
  public:
    StackController(Stack2* stack_) : stack(stack_), updated(false) {}

    void build_rag();
    int remove_inclusions();
    void merge_labels(Label_t label1, Label_t label2, RagNodeCombineAlg* combine_alg);
    
    void dilate_labelvol(int disc_size);
    void dilate_gt_labelvol(int disc_size);

    void compute_vi(double& merge, double& split);    
    void compute_vi(double& merge, double& split, std::multimap<double, Label_t>& label_ranked,
            std::multimap<double, Label_t>& gt_ranked);
 
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

  private:
    struct LabelCount{
        Label lbl;
        size_t count;
        LabelCount(): lbl(0), count(0) {};	 	 		
        LabelCount(Label plbl, size_t pcount): lbl(plbl), count(pcount) {};	 	 		
    };

    VolumeLabelPtr dilate_label_edges(VolumeLabelPtr ptr, int disc_size);
    VolumeLabelPtr generate_boundary(VolumeLabelPtr ptr);   
    void compute_contingency_table();
 
    Stack2* stack;
    bool updated;
    std::tr1::unordered_map<Label_t, std::vector<LabelCount> > contingency;	
};

}

#endif
