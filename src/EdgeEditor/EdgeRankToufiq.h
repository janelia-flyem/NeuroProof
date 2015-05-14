
#ifndef _EDGERANK_TOUFIQ
#define _EDGERANK_TOUFIQ

#include "EdgeRank.h"
#include <utility>

#include <BioPriors/IterativeLearn_semi.h>

namespace NeuroProof{

class EdgeRankToufiq: public EdgeRank{
    
    std::vector<unsigned int> new_idx;  
    std::vector<unsigned int> tmp_idx;  
    std::vector<unsigned int> backup_idx;  
    std::vector<int> new_lbl;  
    std::vector<int> tmp_lbl;  
    
    std::vector< std::pair<Node_t, Node_t> > edgebuffer;
    std::vector< std::pair<Node_t, Node_t> > backupbuffer;

    IterativeLearn_semi* ils;
  
    unsigned int edge_ptr;
    
    unsigned int maxEdges;
    unsigned int subset_sz;

    virtual void update_priority() {};
    
    boost::thread* threadp;
    
    std::vector< boost::tuple<unsigned int, unsigned int, int> > all_labeled_edges;
    
public:
    EdgeRankToufiq(BioStack* pstack, Rag_t& prag, string session_name="");
    
    ~EdgeRankToufiq(){
	if (ils)
	    delete ils;
	if (threadp){
	  threadp->join();
	  delete threadp;
	}
    }
  
    void examined_edge(NodePair node_pair, bool remove) {}; 

    bool get_top_edge(NodePair& top_edge_) ;
    void set_label(int plbl);

    void undo() {};
    
    bool is_finished() ; 
    
    void repopulate_buffer();
    
    unsigned int get_num_remaining() {return (maxEdges-num_processed);};
    
    std::string get_identifier() {};

    unsigned int get_num_processed()
    {
        return num_processed;
    }

    void add_to_edgelist(std::vector< std::pair<Node_t, Node_t> > &pedgebuffer, std::vector<int>& plabels);
    void save_labeled_edges(string& save_fname);
};
}

#endif
