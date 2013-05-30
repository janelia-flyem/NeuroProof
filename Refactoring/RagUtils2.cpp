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

void rag_merge_edge_median(Rag_uit& rag, RagEdge_uit* edge, RagNode_uit* node_keep, MergePriority* priority, FeatureMgr* feature_mgr)
{
    RagNode_uit* node_remove = edge->get_other_node(node_keep);
    feature_mgr->merge_features(node_keep, node_remove);
    
    for(RagNode_uit::edge_iterator iter = node_remove->edge_begin(); iter != node_remove->edge_end(); ++iter) {
        RagNode_uit* other_node = (*iter)->get_other_node(node_remove);
        if (other_node == node_keep) {
            continue;
        }
        
        RagEdge_uit* temp_edge = rag.find_rag_edge(node_keep, other_node);

        bool preserve = (*iter)->is_preserve();
        bool false_edge = (*iter)->is_false_edge();
        if (temp_edge) {
            preserve = preserve || temp_edge->is_preserve(); 
            false_edge = false_edge && temp_edge->is_false_edge(); 
        }

        RagEdge_uit* new_edge = temp_edge;
        if (temp_edge) {
            if (temp_edge->is_false_edge()) {
                feature_mgr->mv_features(*iter, temp_edge); 
            } else if (!((*iter)->is_false_edge())) {
                feature_mgr->merge_features(temp_edge, (*iter));
            } else {
                feature_mgr->remove_edge(*iter);
            }
            //double val = feature_mgr->get_prob(temp_edge);
            //ranking.insert(std::make_pair(val, std::make_pair(temp_edge->get_node1()->get_node_id(), temp_edge->get_node2()->get_node_id())));
        } else {
            new_edge = rag.insert_rag_edge(node_keep, other_node);
            feature_mgr->mv_features(*iter, new_edge); 
	    new_edge->set_edge_id((*iter)->get_edge_id());
            //double val = feature_mgr->get_prob(new_edge);
            //ranking.insert(std::make_pair(val, std::make_pair(new_edge->get_node1()->get_node_id(), new_edge->get_node2()->get_node_id())));
        } 
  
        Node_uit node1 = new_edge->get_node1()->get_node_id();
        Node_uit node2 = new_edge->get_node2()->get_node_id();

        new_edge->set_preserve(preserve); 
        new_edge->set_false_edge(false_edge); 
    }

    //node_keep->set_size(node_keep->get_size() + node_remove->get_size());
    feature_mgr->remove_edge(edge); 
    rag.remove_rag_node(node_remove);

    for(RagNode_uit::edge_iterator iter = node_keep->edge_begin(); iter != node_keep->edge_end(); ++iter) {
        priority->add_dirty_edge(*iter);
    
	RagNode_uit* node = (*iter)->get_other_node(node_keep);
        for(RagNode_uit::edge_iterator iter2 = node->edge_begin(); iter2 != node->edge_end(); ++iter2) {
            priority->add_dirty_edge(*iter2);
        }
    }
}


void rag_merge_edge(Rag_uit& rag, RagEdge_uit* edge, RagNode_uit* node_keep, FeatureMgr* feature_mgr)
{
    RagNode_uit* node_remove = edge->get_other_node(node_keep);
    feature_mgr->merge_features(node_keep, node_remove);
    
    for(RagNode_uit::edge_iterator iter = node_remove->edge_begin(); iter != node_remove->edge_end(); ++iter) {
        RagNode_uit* other_node = (*iter)->get_other_node(node_remove);
        if (other_node == node_keep) {
            continue;
        }
        
        RagEdge_uit* temp_edge = rag.find_rag_edge(node_keep, other_node);

        bool preserve = (*iter)->is_preserve();
        bool false_edge = (*iter)->is_false_edge();
        if (temp_edge) {
            preserve = preserve || temp_edge->is_preserve(); 
            false_edge = false_edge && temp_edge->is_false_edge(); 
        }

        RagEdge_uit* new_edge = temp_edge;
        if (temp_edge) {
            if (temp_edge->is_false_edge()) {
                feature_mgr->mv_features(*iter, temp_edge); 
            } else if (!((*iter)->is_false_edge())) {
                feature_mgr->merge_features(temp_edge, (*iter));
            } else {
                feature_mgr->remove_edge(*iter);
            }
            //double val = feature_mgr->get_prob(temp_edge);
            //ranking.insert(std::make_pair(val, std::make_pair(temp_edge->get_node1()->get_node_id(), temp_edge->get_node2()->get_node_id())));
        } else {
            new_edge = rag.insert_rag_edge(node_keep, other_node);
            feature_mgr->mv_features(*iter, new_edge); 
            //double val = feature_mgr->get_prob(new_edge);
            //ranking.insert(std::make_pair(val, std::make_pair(new_edge->get_node1()->get_node_id(), new_edge->get_node2()->get_node_id())));
        } 
  
        Node_uit node1 = new_edge->get_node1()->get_node_id();
        Node_uit node2 = new_edge->get_node2()->get_node_id();

        new_edge->set_preserve(preserve); 
        new_edge->set_false_edge(false_edge); 
    }

    //node_keep->set_size(node_keep->get_size() + node_remove->get_size());
    
    feature_mgr->remove_edge(edge); 
    rag.remove_rag_node(node_remove);

}

void rag_merge_edge_priorityq(Rag_uit& rag, RagEdge_uit* edge, RagNode_uit* node_keep, MergePriorityQueue<QE>* priority, FeatureMgr* feature_mgr)
{
    RagNode_uit* node_remove = edge->get_other_node(node_keep);

    feature_mgr->merge_features(node_keep, node_remove);
    
    for(RagNode_uit::edge_iterator iter = node_remove->edge_begin(); iter != node_remove->edge_end(); ++iter) {
        RagNode_uit* other_node = (*iter)->get_other_node(node_remove);
        
        if (other_node == node_keep) {
            continue;
        }

        int qloc= (*iter)->get_qloc();	
	if (qloc>=0) {
	    //priority->heap_delete(qloc+1);
	    priority->invalidate(qloc+1);
	}



        RagEdge_uit* temp_edge = rag.find_rag_edge(node_keep, other_node);

        bool preserve = (*iter)->is_preserve();
        bool false_edge = (*iter)->is_false_edge();
        if (temp_edge) {
            preserve = preserve || temp_edge->is_preserve(); 
            false_edge = false_edge && temp_edge->is_false_edge(); 
        }

        RagEdge_uit* new_edge = temp_edge;

	// if edge(node_keep, other_node) already exists
        if (temp_edge) {
            if (temp_edge->is_false_edge()) {
                feature_mgr->mv_features(*iter, temp_edge); 
            } else if (!((*iter)->is_false_edge())) {
                feature_mgr->merge_features(temp_edge, (*iter));
            } else {
                feature_mgr->remove_edge(*iter);
            }
        } 
	else {	// if edge(node_keep, other_node) does not exist

            new_edge = rag.insert_rag_edge(node_keep, other_node);
            feature_mgr->mv_features(*iter, new_edge); 
	    new_edge->set_edge_id((*iter)->get_edge_id());

        } 
  
        new_edge->set_preserve(preserve); 
        new_edge->set_false_edge(false_edge); 
    }

    //node_keep->set_size(node_keep->get_size() + node_remove->get_size());
    
    feature_mgr->remove_edge(edge); 
    rag.remove_rag_node(node_remove);

    // refine prob for all nodes adjacent to node_keep 	
    for(RagNode_uit::edge_iterator iter = node_keep->edge_begin(); iter != node_keep->edge_end(); ++iter) {
        double val = feature_mgr->get_prob(*iter);
	double prev_val = (*iter)->get_weight(); 
	(*iter)->set_weight(val);
	Label node1 = (*iter)->get_node1()->get_node_id();
	Label node2 = (*iter)->get_node2()->get_node_id();

	QE tmpelem(val, std::make_pair(node1,node2));	

	int qloc= (*iter)->get_qloc();	

	if (qloc>=0){
	    if (val<prev_val)
                priority->heap_decrease_key(qloc+1, tmpelem);
	    else if (val>prev_val) 	   	
                priority->heap_increase_key(qloc+1, tmpelem);

	}

	else
	    priority->heap_insert(tmpelem);	

    }
}









void rag_merge_edge_flat(Rag_uit& rag, RagEdge_uit* edge, RagNode_uit* node_keep, std::vector<QE>& priority, FeatureMgr* feature_mgr)
{
    RagNode_uit* node_remove = edge->get_other_node(node_keep);

    feature_mgr->merge_features(node_keep, node_remove);
    //printf("node keep : %d\n",node_keep->get_node_id());
    //printf("node remove : %d\n",node_remove->get_node_id());

    for(RagNode_uit::edge_iterator iter = node_remove->edge_begin(); iter != node_remove->edge_end(); ++iter) {
        RagNode_uit* other_node = (*iter)->get_other_node(node_remove);
        
        if (other_node == node_keep) {
            continue;
        }


	//printf("for edge (%d, %d)  ",(*iter)->get_node1()->get_node_id(), (*iter)->get_node2()->get_node_id());

        RagEdge_uit* temp_edge = rag.find_rag_edge(node_keep, other_node);

        bool preserve = (*iter)->is_preserve();
        bool false_edge = (*iter)->is_false_edge();
        if (temp_edge) {
            preserve = preserve || temp_edge->is_preserve(); 
            false_edge = false_edge && temp_edge->is_false_edge(); 
        }

        RagEdge_uit* new_edge = temp_edge;

	// if edge(node_keep, other_node) already exists
        if (temp_edge) {
	    //printf("  edge (%d, %d) exists\n",temp_edge->get_node1()->get_node_id(), temp_edge->get_node2()->get_node_id());
            if (temp_edge->is_false_edge()) {
		int tt=1;
                feature_mgr->mv_features(*iter, temp_edge); 
            } else if (!((*iter)->is_false_edge())) {
		int tt=1;
                feature_mgr->merge_features(temp_edge, (*iter));
            } else {
                feature_mgr->remove_edge(*iter);
            }
	    double prob = feature_mgr->get_prob(temp_edge);
	    temp_edge->set_weight(prob);	
	
            int qloc= (*iter)->get_qloc();	
	    if (qloc>=0) {
	        priority.at(qloc).invalidate();
	    }
        } 
	else {	// if edge(node_keep, other_node) does not exist

	    double val = (*iter)->get_weight();
            int qloc= (*iter)->get_qloc();
            new_edge = rag.insert_rag_edge(node_keep, other_node);
            feature_mgr->mv_features(*iter, new_edge); 
	    new_edge->set_weight(val);
	    //printf(" creating edge (%d, %d)\n",new_edge->get_node1()->get_node_id(), new_edge->get_node2()->get_node_id());

	    
	    if (qloc>=0){

                QE tmpelem(val, std::make_pair(new_edge->get_node1()->get_node_id(), new_edge->get_node2()->get_node_id()));	
	        tmpelem.reassign_qloc(qloc, &rag); 	
	        priority.at(qloc) = tmpelem; // replace (node_remove, other_node) with (node_keep, other_node)	
	   }
	   else {
		//printf("not in priority queue\n");
	   }			
        } 
  
        new_edge->set_preserve(preserve); 
        new_edge->set_false_edge(false_edge); 
    }

    
    feature_mgr->remove_edge(edge); 
    rag.remove_rag_node(node_remove);

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
