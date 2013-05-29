#ifndef RAGUTILS_H
#define RAGUTILS_H

#include "Rag.h"

#include <map>
#include <string>
#include <iostream>

namespace NeuroProof {


template <typename Region>
unsigned long long get_rag_size(Rag<Region>* rag)
{
    unsigned long long total_num_voxels = 0;
    for (typename Rag<Region>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        total_num_voxels += (*iter)->get_size();
    }
    return total_num_voxels;
}

// take smallest value edge and use that when connecting back
template <typename Region>
void rag_merge_edge(Rag<Region>& rag, RagEdge<Region>* edge, RagNode<Region>* node_keep, std::vector<std::string>& property_names)
{
    RagNode<Region>* node_remove = edge->get_other_node(node_keep);

    for(typename RagNode<Region>::edge_iterator iter = node_remove->edge_begin(); iter != node_remove->edge_end(); ++iter) {
        double weight = (*iter)->get_weight();
        RagNode<Region>* other_node = (*iter)->get_other_node(node_remove);
        if (other_node == node_keep) {
            continue;
        }

        RagEdge<Region>* temp_edge = rag.find_rag_edge(node_keep, other_node);
       
        RagEdge<Region>* final_edge = temp_edge;

        if (temp_edge && ((weight <= temp_edge->get_weight() && (temp_edge->get_weight() <= 1.0)) || (weight > 1.0)) ) { 
            temp_edge->set_weight(weight);
            
            if (!((*iter)->is_false_edge())) {
                for (int i = 0; i < property_names.size(); ++i) {
                    PropertyPtr property = (*iter)->get_property_ptr(property_names[i]);
                    temp_edge->set_property_ptr(property_names[i], property);
                }
            }
        } else if (!temp_edge) {
            RagEdge<Region>* new_edge = rag.insert_rag_edge(node_keep, other_node);
            new_edge->set_weight(weight);
            for (int i = 0; i < property_names.size(); ++i) {
                PropertyPtr property = (*iter)->get_property_ptr(property_names[i]);
                new_edge->set_property_ptr(property_names[i], property);
            }
            final_edge = new_edge;
        }
    
        if ((*iter)->is_preserve()) {
            final_edge->set_preserve(true);
        }

        if ((*iter)->is_false_edge() && (!temp_edge || final_edge->is_false_edge())) {
            final_edge->set_false_edge(true);
        } else {
            final_edge->set_false_edge(false);
        } 
    }

    node_keep->set_size(node_keep->get_size() + node_remove->get_size());
    rag.remove_rag_node(node_remove);     
}


}

#endif
