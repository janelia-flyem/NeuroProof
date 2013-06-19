#ifndef STACKAGGLOMALGS_H
#define STACKAGGLOMALGS_H

namespace NeuroProof {

class StackController;

void agglomerate_stack(StackController& controller, double threshold,
                        bool use_mito, bool use_edge_weight = false, bool synapse_mode=false);

void agglomerate_stack_mrf(StackController& controller, double threshold, bool use_mito);

void agglomerate_stack_queue(StackController& controller, double threshold, 
                                bool use_mito, bool use_edge_weight = false);

void agglomerate_stack_flat(StackController& controller, double threshold, bool use_mito);

void agglomerate_stack_mito(StackController& controller, double threshold=0.8);

}

#endif
