#include "MergePriorityFunction.h"
#include "../BioPriors/MitoTypeProperty.h"

#include <cstdio>

using namespace NeuroProof;

double mito_boundary_ratio(RagEdge_t* edge)
{
    RagNode_t* node1 = edge->get_node1();
    RagNode_t* node2 = edge->get_node2();
    double ratio = 0.0;

    try {
        MitoTypeProperty& type1_mito = node1->get_property<MitoTypeProperty>("mito-type");
        MitoTypeProperty& type2_mito = node2->get_property<MitoTypeProperty>("mito-type");
        int type1 = type1_mito.get_node_type(); 
        int type2 = type2_mito.get_node_type(); 

        RagNode_t* mito_node = 0;		
        RagNode_t* other_node = 0;		

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



void ProbPriority::initialize_priority(double threshold_, bool use_edge_weight)
{
    threshold = threshold_;
    for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
	if (valid_edge(*iter)) {
	    double val;
	    if (use_edge_weight)
		val = (*iter)->get_weight();
	    else
		val = feature_mgr->get_prob(*iter);
	    (*iter)->set_weight(val);

	    if (val <= threshold) {
		ranking.insert(std::make_pair(val, std::make_pair((*iter)->get_node1()->get_node_id(), (*iter)->get_node2()->get_node_id())));
	    }
	}
    }
}



void ProbPriority::initialize_random(double pthreshold){

    threshold = pthreshold;
    for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
	if (valid_edge(*iter)) {

	    double val1 = feature_mgr->get_prob(*iter);
	    (*iter)->set_weight(val1);

	    if (val1 <= threshold){ 
		srand ( time(NULL) );
		double val= rand()*(threshold/ RAND_MAX);

		(*iter)->set_weight(val);
		ranking.insert(std::make_pair(val, std::make_pair((*iter)->get_node1()->get_node_id(), (*iter)->get_node2()->get_node_id())));
	    }
	}
    }
}
   
void ProbPriority::clear_dirty()
{
    for (Dirty_t::iterator iter = dirty_edges.begin(); iter != dirty_edges.end(); ++iter) {
	Node_t node1 = (*iter).region1;
	Node_t node2 = (*iter).region2;
	RagNode_t* rag_node1 = rag->find_rag_node(node1); 
	RagNode_t* rag_node2 = rag->find_rag_node(node2); 

	if (!(rag_node1 && rag_node2)) {
	    continue;
	}
	RagEdge_t* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

	if (!rag_edge) {
	    continue;
	}

	assert(rag_edge->is_dirty());
	rag_edge->set_dirty(false);

	if (valid_edge(rag_edge)) {
	    double val = feature_mgr->get_prob(rag_edge);
	    rag_edge->set_weight(val);

	    if (val <= threshold) {
		ranking.insert(std::make_pair(val, std::make_pair(node1, node2)));
	    }
	    else{ 
		kicked_out++;	
		if (kicked_fid)
		  fprintf(kicked_fid, "0 %f %u %u %lu %lu\n", val,
		    node1, node2, rag_node1->get_size(), rag_node2->get_size());
	    }
	}
    }
    dirty_edges.clear();
}

bool ProbPriority::empty()
{
    if (ranking.empty()) {
	clear_dirty();
    }
    return ranking.empty();
}


RagEdge_t* ProbPriority::get_top_edge()
{
    EdgeRank_t::iterator first_entry = ranking.begin();
    double curr_threshold = (*first_entry).first;
    Node_t node1 = (*first_entry).second.first;
    Node_t node2 = (*first_entry).second.second;
    ranking.erase(first_entry);

    //cout << curr_threshold << " " << node1 << " " << node2 << std::endl;

    if (curr_threshold > threshold) {
	ranking.clear();
	return 0;
    }

    RagNode_t* rag_node1 = rag->find_rag_node(node1); 
    RagNode_t* rag_node2 = rag->find_rag_node(node2); 

    if (!(rag_node1 && rag_node2)) {
	return 0;
    }
    RagEdge_t* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

    if (!rag_edge) {
	return 0;
    }

    if (!valid_edge(rag_edge)) {
	return 0;
    }

    double val = rag_edge->get_weight();

    bool dirty = false;
    if (rag_edge->is_dirty()) {
	dirty = true;
	val = feature_mgr->get_prob(rag_edge);
	rag_edge->set_weight(val);
	rag_edge->set_dirty(false);
	dirty_edges.erase(OrderedPair(node1, node2));
    }

    if (val > (curr_threshold + Epsilon)) {
	if (dirty && (val <= threshold)) {
	    ranking.insert(std::make_pair(val, std::make_pair(node1, node2)));
	}
	else{ 
	    //printf("edge prob changed from %.4f to %.4f\n",curr_threshold, val);
	    kicked_out++;	
	    if (kicked_fid)
	      fprintf(kicked_fid, "0 %f %u %u %lu %lu\n", val,
		node1, node2, rag_node1->get_size(), rag_node2->get_size());
	    //fprintf(kicked_fid, "0 %f %u %u %lu %lu\n", rag_edge->get_weight(),node1, node2, rag_node1->get_size(), rag_node2->get_size());
	    
	    //add_dirty_edge(rag_edge);
	}
	return 0;
    }
    return rag_edge; 
}

void ProbPriority::add_dirty_edge(RagEdge_t* edge)
{
    if (valid_edge(edge)) {
	edge->set_dirty(true);
	dirty_edges.insert(OrderedPair(edge->get_node1()->get_node_id(), edge->get_node2()->get_node_id()));
    }
}









//*******************************************************************************************************************



void MitoPriority::initialize_priority(double threshold_, bool use_edge_weight)
{
    threshold = threshold_;
    for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if (valid_edge(*iter)) {
	    double val;
	    val = 1 - mito_boundary_ratio((*iter));

	    if (val < threshold) {
	        ranking.insert(std::make_pair(val, std::make_pair((*iter)->get_node1()->get_node_id(), (*iter)->get_node2()->get_node_id())));
	    }
        }
    }
}




void MitoPriority::clear_dirty()
{
    for (Dirty_t::iterator iter = dirty_edges.begin(); iter != dirty_edges.end(); ++iter) {
	Node_t node1 = (*iter).region1;
	Node_t node2 = (*iter).region2;
	RagNode_t* rag_node1 = rag->find_rag_node(node1); 
	RagNode_t* rag_node2 = rag->find_rag_node(node2); 

	if (!(rag_node1 && rag_node2)) {
	    continue;
	}
	RagEdge_t* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

	if (!rag_edge) {
	    continue;
	}

	assert(rag_edge->is_dirty());
	rag_edge->set_dirty(false);

	if (valid_edge(rag_edge)) {
	    double val = 1 - mito_boundary_ratio(rag_edge);

	    if (val < threshold) {
		ranking.insert(std::make_pair(val, std::make_pair(node1, node2)));
	    }
	}
    }
    dirty_edges.clear();
}

bool MitoPriority::empty()
{
    if (ranking.empty()) {
	clear_dirty();
    }
    return ranking.empty();
}


RagEdge_t* MitoPriority::get_top_edge()
{
    EdgeRank_t::iterator first_entry = ranking.begin();
    double curr_threshold = (*first_entry).first;
    Node_t node1 = (*first_entry).second.first;
    Node_t node2 = (*first_entry).second.second;
    ranking.erase(first_entry);

    //cout << curr_threshold << " " << node1 << " " << node2 << std::endl;

    if (curr_threshold >= threshold) {
	ranking.clear();
	return 0;
    }

    RagNode_t* rag_node1 = rag->find_rag_node(node1); 
    RagNode_t* rag_node2 = rag->find_rag_node(node2); 

    if (!(rag_node1 && rag_node2)) {
	return 0;
    }
    RagEdge_t* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

    if (!rag_edge) {
	return 0;
    }

    if (!valid_edge(rag_edge)) {
	return 0;
    }

    double val = 1 - mito_boundary_ratio(rag_edge);

    bool dirty = false;
    if (rag_edge->is_dirty()) {
	dirty = true;
	val = 1 - mito_boundary_ratio(rag_edge);
	rag_edge->set_dirty(false);
	dirty_edges.erase(OrderedPair(node1, node2));
    }

    if (val > (curr_threshold + Epsilon)) {
	if (dirty && (val < threshold)) {
	    ranking.insert(std::make_pair(val, std::make_pair(node1, node2)));
	}
	else{ 
	    //printf("edge prob changed from %.4f to %.4f\n",curr_threshold, val);
	    kicked_out++;	
	    //add_dirty_edge(rag_edge);
	}
	return 0;
    }
    return rag_edge; 
}

void MitoPriority::add_dirty_edge(RagEdge_t* edge)
{
    if (valid_edge(edge)) {
	edge->set_dirty(true);
	dirty_edges.insert(OrderedPair(edge->get_node1()->get_node_id(), edge->get_node2()->get_node_id()));
    }
}

