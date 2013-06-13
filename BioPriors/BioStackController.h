#ifndef BIOSTACKCONTROLLER_H
#define BIOSTACKCONTROLLER_H

#include "../Stack/StackController.h"
#include "BioStack.h"

namespace NeuroProof {

class BioStackController : public StackController {
  public:
    BioStackController(BioStack* biostack_) : StackController(biostack_),
            biostack(biostack_) {}
    VolumeLabelPtr create_syn_label_volume();
    VolumeLabelPtr create_syn_gt_label_volume();
    void set_synapse_exclusions(const char * synapse_json);    

    void build_rag_mito();

  private:
    void add_edge_constraint(RagPtr rag, VolumeLabelPtr labelvol, unsigned int x1,
            unsigned int y1, unsigned int z1, unsigned int x2, unsigned int y2, unsigned int z2);
    VolumeLabelPtr create_syn_volume(VolumeLabelPtr labelvol);
    
    BioStack* biostack;
};

}

#endif
