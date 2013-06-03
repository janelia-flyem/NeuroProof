#include "RagUtils2.h"

namespace NeuroProof {

void rag_add_edge(Rag_uit* rag, unsigned int id1, unsigned int id2, std::vector<double>& preds, 
        FeatureMgr * feature_mgr)
{
    RagNode_uit * node1 = rag->find_rag_node(id1);
    if (!node1) {
        node1 = rag->insert_rag_node(id1);
    }
    
    RagNode_uit * node2 = rag->find_rag_node(id2);
    if (!node2) {
        node2 = rag->insert_rag_node(id2);
    }
   
    assert(node1 != node2);

    RagEdge_uit* edge = rag->find_rag_edge(node1, node2);
    if (!edge) {
        edge = rag->insert_rag_edge(node1, node2);
    }

    if (feature_mgr) {
        feature_mgr->add_val(preds, edge);
    } else {
        edge->incr_size();
    }
}  

double mito_boundary_ratio(RagEdge_uit* edge)
{
    RagNode_uit* node1 = edge->get_node1();
    RagNode_uit* node2 = edge->get_node2();
    double ratio = 0.0;

    try {
        MitoTypeProperty& type1_mito = node1->get_property<MitoTypeProperty>("mito-type");
        MitoTypeProperty& type2_mito = node2->get_property<MitoTypeProperty>("mito-type");
        int type1 = type1_mito.get_node_type(); 
        int type2 = type2_mito.get_node_type(); 

        RagNode_uit* mito_node = 0;		
        RagNode_uit* other_node = 0;		

        if ((type1 == 2) && (type2 == 1) ){
            mito_node = node1;
            other_node = node2;
        } else if((type2 == 2) && (type1 == 1) ){
            mito_node = node2;
            other_node = node1;
        } else { 
            return 0.0; 	
        }

        if (mito_node->get_size() > other_node->get_size()) {
            return 0.0;
        }

        unsigned long long mito_node_border_len = mito_node->compute_border_length();		

        ratio = (edge->get_size())*1.0/mito_node_border_len; 

        if (ratio > 1.0){
            printf("ratio > 1 for %d %d\n", mito_node->get_node_id(), other_node->get_node_id());
            return 0.0;
        }

    } catch (ErrMsg& err) {
        // # just return 
    } 

    return ratio;
}


// assume that 0 body will never be added as a screen
EdgeRanking::type rag_grab_edge_ranking(Rag_uit& rag, double min_val, double max_val, double start_val, double ignore_size, Node_uit screen_id)
{
    EdgeRanking::type ranking;

    for (Rag_uit::edges_iterator iter = rag.edges_begin(); iter != rag.edges_end(); ++iter) {
        if ((screen_id != 0) && ((*iter)->get_node1()->get_node_id() != screen_id) && ((*iter)->get_node2()->get_node_id() != screen_id)) {
            continue;
        }
        
        double val = (*iter)->get_weight();
        if ((val <= max_val) && (val >= min_val)) {
            if (((*iter)->get_node1()->get_size() > ignore_size) && ((*iter)->get_node2()->get_size() > ignore_size)) {
                ranking.insert(std::make_pair(std::abs(val - start_val), *iter));
            }
        } 
    }

    return ranking;
}

}
