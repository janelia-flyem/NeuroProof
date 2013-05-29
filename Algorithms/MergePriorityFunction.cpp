#include "MergePriorityFunction.h"
#include "../Refactoring/RagUtils2.h"

#include <cstdio>

using namespace NeuroProof;

void ProbPriority::initialize_priority(double threshold_, bool use_edge_weight)
{
    threshold = threshold_;
    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
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
    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
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
	Label node1 = (*iter).region1;
	Label node2 = (*iter).region2;
	RagNode<Label>* rag_node1 = rag->find_rag_node(node1); 
	RagNode<Label>* rag_node2 = rag->find_rag_node(node2); 

	if (!(rag_node1 && rag_node2)) {
	    continue;
	}
	RagEdge<Label>* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

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
		  fprintf(kicked_fid, "%u 0 %f %u %u %lu %lu\n",rag_edge->get_edge_id(),  val,
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


RagEdge<Label>* ProbPriority::get_top_edge()
{
    EdgeRank_t::iterator first_entry = ranking.begin();
    double curr_threshold = (*first_entry).first;
    Label node1 = (*first_entry).second.first;
    Label node2 = (*first_entry).second.second;
    ranking.erase(first_entry);

    //cout << curr_threshold << " " << node1 << " " << node2 << std::endl;

    if (curr_threshold > threshold) {
	ranking.clear();
	return 0;
    }

    RagNode<Label>* rag_node1 = rag->find_rag_node(node1); 
    RagNode<Label>* rag_node2 = rag->find_rag_node(node2); 

    if (!(rag_node1 && rag_node2)) {
	return 0;
    }
    RagEdge<Label>* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

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
	dirty_edges.erase(OrderedPair<Label>(node1, node2));
    }

    if (val > (curr_threshold + Epsilon)) {
	if (dirty && (val <= threshold)) {
	    ranking.insert(std::make_pair(val, std::make_pair(node1, node2)));
	}
	else{ 
	    //printf("edge prob changed from %.4f to %.4f\n",curr_threshold, val);
	    kicked_out++;	
	    if (kicked_fid)
	      fprintf(kicked_fid, "%u 0 %f %u %u %lu %lu\n",rag_edge->get_edge_id(),  val,
		node1, node2, rag_node1->get_size(), rag_node2->get_size());
	    //fprintf(kicked_fid, "0 %f %u %u %lu %lu\n", rag_edge->get_weight(),node1, node2, rag_node1->get_size(), rag_node2->get_size());
	    
	    //add_dirty_edge(rag_edge);
	}
	return 0;
    }
    return rag_edge; 
}

void ProbPriority::add_dirty_edge(RagEdge<Label>* edge)
{
    if (valid_edge(edge)) {
	edge->set_dirty(true);
	dirty_edges.insert(OrderedPair<Label>(edge->get_node1()->get_node_id(), edge->get_node2()->get_node_id()));
    }
}









//*******************************************************************************************************************



void MitoPriority::initialize_priority(double threshold_, bool use_edge_weight)
{
    threshold = threshold_;
    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
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
	Label node1 = (*iter).region1;
	Label node2 = (*iter).region2;
	RagNode<Label>* rag_node1 = rag->find_rag_node(node1); 
	RagNode<Label>* rag_node2 = rag->find_rag_node(node2); 

	if (!(rag_node1 && rag_node2)) {
	    continue;
	}
	RagEdge<Label>* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

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


RagEdge<Label>* MitoPriority::get_top_edge()
{
    EdgeRank_t::iterator first_entry = ranking.begin();
    double curr_threshold = (*first_entry).first;
    Label node1 = (*first_entry).second.first;
    Label node2 = (*first_entry).second.second;
    ranking.erase(first_entry);

    //cout << curr_threshold << " " << node1 << " " << node2 << std::endl;

    if (curr_threshold >= threshold) {
	ranking.clear();
	return 0;
    }

    RagNode<Label>* rag_node1 = rag->find_rag_node(node1); 
    RagNode<Label>* rag_node2 = rag->find_rag_node(node2); 

    if (!(rag_node1 && rag_node2)) {
	return 0;
    }
    RagEdge<Label>* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

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
	dirty_edges.erase(OrderedPair<Label>(node1, node2));
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

void MitoPriority::add_dirty_edge(RagEdge<Label>* edge)
{
    if (valid_edge(edge)) {
	edge->set_dirty(true);
	dirty_edges.insert(OrderedPair<Label>(edge->get_node1()->get_node_id(), edge->get_node2()->get_node_id()));
    }
}

