#include "RagUtils.h"

namespace NeuroProof {

// take smallest value edge and use that when connecting back
void rag_merge_edge(Rag_uit& rag, RagEdge_uit* edge, RagNode_uit* node_keep, std::vector<std::string>& property_names)
{
    RagNode_uit* node_remove = edge->get_other_node(node_keep);

    for(typename RagNode_uit::edge_iterator iter = node_remove->edge_begin(); iter != node_remove->edge_end(); ++iter) {
        double weight = (*iter)->get_weight();
        RagNode_uit* other_node = (*iter)->get_other_node(node_remove);
        if (other_node == node_keep) {
            continue;
        }

        RagEdge_uit* temp_edge = rag.find_rag_edge(node_keep, other_node);
       
        RagEdge_uit* final_edge = temp_edge;

        if (temp_edge && ((weight <= temp_edge->get_weight() && (temp_edge->get_weight() <= 1.0)) || (weight > 1.0)) ) { 
            temp_edge->set_weight(weight);
            
            if (!((*iter)->is_false_edge())) {
                for (int i = 0; i < property_names.size(); ++i) {
                    PropertyPtr property = (*iter)->get_property_ptr(property_names[i]);
                    temp_edge->set_property_ptr(property_names[i], property);
                }
            }
        } else if (!temp_edge) {
            RagEdge_uit* new_edge = rag.insert_rag_edge(node_keep, other_node);
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
