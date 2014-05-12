#include "EdgeRankToufiq.h"


using namespace NeuroProof;

EdgeRankToufiq::EdgeRankToufiq(BioStack* pstack, Rag_t& prag): EdgeRank(&prag)
{
    ils = new IterativeLearn_semi(pstack);
    maxEdges = 5000;
    subset_sz= 10;

    num_processed = 0;
    ils->get_initial_edges(new_idx);
    ils->edgelist_from_index(new_idx, edgebuffer);
    new_lbl.resize(edgebuffer.size());
    edge_ptr = 0;
    printf("Edge Rank initialized\n");
    
    ils->get_extra_edges(backup_idx, subset_sz);
    ils->edgelist_from_index(backup_idx, backupbuffer);
    
    threadp = NULL;
    
};


bool EdgeRankToufiq::get_top_edge(NodePair& top_edge_){
    if(edgebuffer.size() < 1)
	return false;
    
    std::pair<Node_t, Node_t> pair1 = edgebuffer[edge_ptr];
    Node_t node1 = pair1.first;
    Node_t node2 = pair1.second;
    boost::get<0>(top_edge_) =  node1; 
    boost::get<1>(top_edge_) =  node2; 
    
    return true;
}
void EdgeRankToufiq::repopulate_buffer()
{
  
    printf("repopulate buffer\n");
    ils->update_new_labels(tmp_idx, tmp_lbl);
    ils->get_next_edge_set(subset_sz, backup_idx);
    ils->edgelist_from_index(backup_idx, backupbuffer);
    
}
bool EdgeRankToufiq::is_finished(){
    
    if(num_processed>=maxEdges)
	return true;
    if (edge_ptr >= edgebuffer.size()){
	if (new_lbl.size() != new_idx.size()){
	    printf("size different: new_idx, new_lbl\n");
	    return false;
	}
// 	ils->update_new_labels(new_idx, new_lbl);
	tmp_idx = new_idx;
	tmp_lbl = new_lbl;
	
	if (threadp)
	  threadp->join();
	edgebuffer = backupbuffer;
	new_idx = backup_idx;
	new_lbl.resize(edgebuffer.size());
	edge_ptr = 0;
	if (edgebuffer.size()<1)
	    return true;
	if (threadp)
	  delete threadp;  
// 	repopulate_buffer();
	threadp = new boost::thread(&EdgeRankToufiq::repopulate_buffer, this);
	
   }
   return false;
}

void EdgeRankToufiq::set_label(int plbl) {
  
    printf("egde %d labeled as %d\n",num_processed,plbl);
    new_lbl[edge_ptr++] = plbl; 
    num_processed++;
  
};
