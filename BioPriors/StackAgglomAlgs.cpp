#include "../Algorithms/MergePriorityFunction.h"
#include "StackAgglomAlgs.h"
#include "../Stack/Stack.h"
#include "MitoTypeProperty.h"
#include "../Algorithms/FeatureJoinAlgs.h"

#include <vector>

using std::vector;

namespace NeuroProof {

bool is_mito(RagNode_t* rag_node)
{
    MitoTypeProperty mtype;
    try {
        mtype = rag_node->get_property<MitoTypeProperty>("mito-type");
    } catch (ErrMsg& msg) {
    }

    if ((mtype.get_node_type()==2)) {	
        return true;
    }
    return false;
}



void agglomerate_stack(Stack& stack, double threshold,
                        bool use_mito, bool use_edge_weight, bool synapse_mode)
{
    if (threshold == 0.0) {
        return;
    }

    RagPtr rag = stack.get_rag();
    FeatureMgrPtr feature_mgr = stack.get_feature_manager();

    MergePriority* priority = new ProbPriority(feature_mgr.get(), rag.get(), synapse_mode);
    priority->initialize_priority(threshold, use_edge_weight);
    DelayedPriorityCombine node_combine_alg(feature_mgr.get(), rag.get(), priority); 
    
    while (!(priority->empty())) {
        RagEdge_t* rag_edge = priority->get_top_edge();

        if (!rag_edge) {
            continue;
        }

        RagNode_t* rag_node1 = rag_edge->get_node1();
        RagNode_t* rag_node2 = rag_edge->get_node2();

        if (use_mito) {
            if (is_mito(rag_node1) || is_mito(rag_node2)) {
                continue;
            }
        }

        Node_t node1 = rag_node1->get_node_id(); 
        Node_t node2 = rag_node2->get_node_id();
        
        // retain node1 
        stack.merge_labels(node2, node1, &node_combine_alg);
    }

    delete priority;
}

void agglomerate_stack_mrf(Stack& stack, double threshold, bool use_mito)
{
    if (threshold == 0.0) {
        return;
    }

    agglomerate_stack(stack, 0.06, use_mito); //0.01 for 250, 0.02 for 500
    stack.remove_inclusions();	  	
    cout <<  "Remaining regions: " << stack.get_num_labels();	

    RagPtr rag = stack.get_rag();
    FeatureMgrPtr feature_mgr = stack.get_feature_manager();

    unsigned int edgeCount=0;	

    for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {
	    double prev_val = (*iter)->get_weight();	
            double val = feature_mgr->get_prob(*iter);
            (*iter)->set_weight(val);

            (*iter)->set_property("qloc", edgeCount);

	    Node_t node1 = (*iter)->get_node1()->get_node_id();	
	    Node_t node2 = (*iter)->get_node2()->get_node_id();	

	    edgeCount++;
	}
    }

    agglomerate_stack(stack, threshold, use_mito, true);
}

void agglomerate_stack_queue(Stack& stack, double threshold, 
                                bool use_mito, bool use_edge_weight)
{
    if (threshold == 0.0) {
        return;
    }

    RagPtr rag = stack.get_rag();
    FeatureMgrPtr feature_mgr = stack.get_feature_manager();

    vector<QE> all_edges;	    	
    int count=0; 	
    for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {


            RagNode_t* rag_node1 = (*iter)->get_node1();
            RagNode_t* rag_node2 = (*iter)->get_node2();

            Node_t node1 = rag_node1->get_node_id(); 
            Node_t node2 = rag_node2->get_node_id(); 

            double val;
            if(use_edge_weight)
                val = (*iter)->get_weight();
            else	
                val = feature_mgr->get_prob(*iter);    

            (*iter)->set_weight(val);
            (*iter)->set_property("qloc", count);

            QE tmpelem(val, make_pair(node1,node2));	
            all_edges.push_back(tmpelem); 

            count++;
        }
    }

    double error=0;  	

    MergePriorityQueue<QE> *Q = new MergePriorityQueue<QE>(rag.get());
    Q->set_storage(&all_edges);	

    PriorityQCombine node_combine_alg(feature_mgr.get(), rag.get(), Q); 

    while (!Q->is_empty()){
        QE tmpqe = Q->heap_extract_min();	

        //RagEdge_t* rag_edge = tmpqe.get_val();
        Node_t node1 = tmpqe.get_val().first;
        Node_t node2 = tmpqe.get_val().second;
        RagEdge_t* rag_edge = rag->find_rag_edge(node1,node2);;

        if (!rag_edge || !tmpqe.valid()) {
            continue;
        }
        double prob = tmpqe.get_key();
        if (prob>threshold)
            break;	

        RagNode_t* rag_node1 = rag_edge->get_node1();
        RagNode_t* rag_node2 = rag_edge->get_node2();
        node1 = rag_node1->get_node_id(); 
        node2 = rag_node2->get_node_id(); 

        if (use_mito) {
            if (is_mito(rag_node1) || is_mito(rag_node2)) {
                continue;
            }
        }

        // retain node1 
        stack.merge_labels(node2, node1, &node_combine_alg);
    }		



}

void agglomerate_stack_flat(Stack& stack, double threshold, bool use_mito)
{
    if (threshold == 0.0) {
        return;
    }

    RagPtr rag = stack.get_rag();
    FeatureMgrPtr feature_mgr = stack.get_feature_manager();

    vector<QE> priority;	    	
    FlatCombine node_combine_alg(feature_mgr.get(), rag.get(), &priority); 
    
    for(int ii=0; ii< priority.size(); ++ii) {
	QE tmpqe = priority[ii];	
        Node_t node1 = tmpqe.get_val().first;
        Node_t node2 = tmpqe.get_val().second;
	if(node1==node2)
	    continue;
	
        RagEdge_t* rag_edge = rag->find_rag_edge(node1,node2);;

        if (!rag_edge || !(priority[ii].valid()) || (rag_edge->get_weight())>threshold ) {
            continue;
        }

        RagNode_t* rag_node1 = rag_edge->get_node1();
        RagNode_t* rag_node2 = rag_edge->get_node2();
        if (use_mito) {
            if (is_mito(rag_node1) || is_mito(rag_node2)) {
                continue;
            }
        }
        
        node1 = rag_node1->get_node_id(); 
        node2 = rag_node2->get_node_id(); 
        
        stack.merge_labels(node2, node1, &node_combine_alg);
    }
}

void agglomerate_stack_mito(Stack& stack, double threshold)
{
    double error=0;  	

    RagPtr rag = stack.get_rag();
    FeatureMgrPtr feature_mgr = stack.get_feature_manager();

    MergePriority* priority = new MitoPriority(feature_mgr.get(), rag.get());
    priority->initialize_priority(threshold);
    
    DelayedPriorityCombine node_combine_alg(feature_mgr.get(), rag.get(), priority); 

    while (!(priority->empty())) {
        RagEdge_t* rag_edge = priority->get_top_edge();

        if (!rag_edge) {
            continue;
        }

        RagNode_t* rag_node1 = rag_edge->get_node1();
        RagNode_t* rag_node2 = rag_edge->get_node2();

        MitoTypeProperty mtype1, mtype2;
	try {    
            mtype1 = rag_node1->get_property<MitoTypeProperty>("mito-type");
        } catch (ErrMsg& msg) {
        
        }
        try {    
            mtype2 = rag_node2->get_property<MitoTypeProperty>("mito-type");
        } catch (ErrMsg& msg) {
        
        }
        if ((mtype1.get_node_type()==2) && (mtype2.get_node_type()==1))	{
            RagNode_t* tmp = rag_node1;
            rag_node1 = rag_node2;
            rag_node2 = tmp;		
        } else if ((mtype2.get_node_type()==2) && (mtype1.get_node_type()==1))	{
            // nophing	
        } else {
            continue;
        }

        Node_t node1 = rag_node1->get_node_id(); 
        Node_t node2 = rag_node2->get_node_id();

        // retain node1 
        stack.merge_labels(node2, node1, &node_combine_alg);
    }

    delete priority;
}

}
