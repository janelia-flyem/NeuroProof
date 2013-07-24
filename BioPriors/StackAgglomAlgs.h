#ifndef STACKAGGLOMALGS_H
#define STACKAGGLOMALGS_H

namespace NeuroProof {

class Stack;

void agglomerate_stack(Stack& stack, double threshold,
                        bool use_mito, bool use_edge_weight = false, bool synapse_mode=false);

void agglomerate_stack_mrf(Stack& stack, double threshold, bool use_mito);

void agglomerate_stack_queue(Stack& stack, double threshold, 
                                bool use_mito, bool use_edge_weight = false);

void agglomerate_stack_flat(Stack& stack, double threshold, bool use_mito);

void agglomerate_stack_mito(Stack& stack, double threshold=0.8);

}

#endif
