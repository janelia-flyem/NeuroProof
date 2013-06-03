
#include "Stack.h"
#include <algorithm>
#include "../Algorithms/FeatureJoinAlgs.h"

using namespace NeuroProof;



void StackLearn::learn_edge_classifier_flat(double threshold, UniqueRowFeature_Label& all_featuresu, std::vector<int>& all_labels, const char* clfr_filename){

    printf("Building RAG ..."); 	
    build_rag();
    printf("done with %d regions\n", get_num_bodies());	


    printf("Inclusion removal ..."); 
    remove_inclusions();
    printf("done with %d regions\n",get_num_bodies());	


    printf("gt label counting\n");


    compute_groundtruth_assignment(); 	



    int count=0; 	
    for (Rag_uit::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {

        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {
	    RagEdge_uit* rag_edge = *iter; 	

            RagNode_uit* rag_node1 = rag_edge->get_node1();
            RagNode_uit* rag_node2 = rag_edge->get_node2();
            Label node1 = rag_node1->get_node_id(); 
            Label node2 = rag_node2->get_node_id(); 

	    int edge_label = decide_edge_label(rag_node1, rag_node2);

	
 	    unsigned long long node1sz = rag_node1->get_size();	
	    unsigned long long node2sz = rag_node2->get_size();	

	    if ( edge_label ){	
		std::vector<double> feature;
		feature_mgr->compute_all_features(rag_edge,feature);


		feature.push_back(edge_label);
		all_featuresu.insert(feature);

	    }	
	    else 
		int checkp = 1;	

        }
    }

    std::vector< std::vector<double> > all_features;
    all_featuresu.get_feature_label(all_features, all_labels); 	    	


    printf("Features generated\n");
    feature_mgr->get_classifier()->learn(all_features, all_labels); // number of trees
    printf("Classifier learned \n");

    /*debug*/
    double err=0;
    for(int fcount=0;fcount<all_features.size(); fcount++){
	double predp = feature_mgr->get_classifier()->predict(all_features[fcount]);
	int predl = (predp>0.5)? 1:-1;	
	err+= ((predl==all_labels[fcount])?0:1);	
    }
    printf("accuracy = %.3f\n",100*(1 - err/all_labels.size()));	
    /**/
    if(clfr_filename){
	feature_mgr->get_classifier()->save_classifier(clfr_filename);
	printf("Classifier saved to %s\n",clfr_filename);
    }

}

// **************************************************************************************************


// **************************************************************************************************


// *C* accumulates the misclassified samples into the training set

void StackLearn::learn_edge_classifier_queue(double threshold, UniqueRowFeature_Label& all_featuresu, std::vector<int>& all_labels, bool accumulate_all, const char* clfr_filename){

    printf("Building RAG ..."); 	
    build_rag();
    printf("done with %d regions\n", get_num_bodies());	

    printf("Inclusion removal ..."); 
    remove_inclusions();
    printf("done with %d regions\n",get_num_bodies());	

    int edge_label;
    compute_groundtruth_assignment(); 	



    std::vector<QE> all_edges;	    	

    int count=0; 	
    for (Rag_uit::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {
            double val = feature_mgr->get_prob(*iter);
            (*iter)->set_weight(val);
	    (*iter)->reassign_qloc(count);
	    Label node1 = (*iter)->get_node1()->get_node_id();	
	    Label node2 = (*iter)->get_node2()->get_node_id();	

            QE tmpelem(val,std::make_pair(node1, node2));	
	    all_edges.push_back(tmpelem); 
	
  	    count++;
        }
    }

    MergePriorityQueue<QE> *Q = new MergePriorityQueue<QE>(rag);
    Q->set_storage(&all_edges);	

    clock_t start = clock();
    PriorityQCombine node_combine_alg(feature_mgr, rag, Q); 

    while (!Q->is_empty()){
	QE tmpqe = Q->heap_extract_min();	


	Label node1 = tmpqe.get_val().first;
	Label node2 = tmpqe.get_val().second;
        RagEdge_uit* rag_edge = rag->find_rag_edge(node1, node2);


        if (!rag_edge || !tmpqe.valid()) {
            continue;
        }
        RagNode_uit* rag_node1 = rag_edge->get_node1();
        RagNode_uit* rag_node2 = rag_edge->get_node2();

        node1 = rag_node1->get_node_id(); 
        node2 = rag_node2->get_node_id(); 


       edge_label = decide_edge_label(rag_node1, rag_node2);

	unsigned long long node1sz = rag_node1->get_size();	
	unsigned long long node2sz = rag_node2->get_size();	


	if  (edge_label){  
	    std::vector<double> feature;
	    feature_mgr->compute_all_features(rag_edge,feature);
	    

	    if(accumulate_all){ 	

		feature.push_back(edge_label);
	        all_featuresu.insert(feature);
	    }	
 	    else if (feature_mgr->get_classifier()->is_trained()){ 		
	        double predp = feature_mgr->get_classifier()->predict(feature);		 
	        int predl = (predp > threshold)? 1:-1;	
		if (predl!=edge_label){

		    feature.push_back(edge_label);
	            all_featuresu.insert(feature);
		}
	    }
	}	
        else 
	    int checkp = 1;	

	if ( edge_label == -1 ){ //merge
	    modify_assignment_after_merge(node1, node2);  	

            rag_join_nodes(*rag, rag_node1, rag_node2, &node_combine_alg);  
	    watershed_to_body[node2] = node1;
	    for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
		watershed_to_body[*iter] = node1;
	    }
	    merge_history[node1].push_back(node2);
	    merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
	    merge_history.erase(node2);
	}

    }

    std::vector< std::vector<double> >	all_features;
    all_featuresu.get_feature_label(all_features, all_labels);	

    printf("Features generated in %.2fsecs\n",((double)clock() - start) / CLOCKS_PER_SEC);
    feature_mgr->get_classifier()->learn(all_features, all_labels); // number of trees
    printf("Classifier learned \n");

    /*debug*/
    double err=0;
    for(int fcount=0;fcount<all_features.size(); fcount++){
	double predp = feature_mgr->get_classifier()->predict(all_features[fcount]);
	int predl = (predp>0.5)? 1:-1;	
	err += ((predl==all_labels[fcount])?0:1);	
    }
    printf("accuracy = %.3f\n",100*(1 - err/all_labels.size()));	
    /**/

    if(clfr_filename){
	feature_mgr->get_classifier()->save_classifier(clfr_filename);
	printf("Classifier saved to %s\n",clfr_filename);
    }

}


//************************************************************************

// *C* does not accumulate new samples, each time the (already learned) classifier generates a merge order
// *C* follow the order, build training set on the way

void StackLearn::learn_edge_classifier_lash(double threshold, UniqueRowFeature_Label& all_featuresu, std::vector<int>& all_labels, const char* clfr_filename){

    printf("Building RAG ..."); 	
    build_rag();
    printf("done with %d regions\n", get_num_bodies());	

    printf("Inclusion removal ..."); 
    remove_inclusions();
    printf("done with %d regions\n",get_num_bodies());	

    int edge_label;
    
    compute_groundtruth_assignment(); 	

    all_featuresu.clear();
    all_labels.clear();	


    std::vector<QE> all_edges;	    	

    int count=0; 	
    for (Rag_uit::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {
            double val = feature_mgr->get_prob(*iter);
            (*iter)->set_weight(val);
	    (*iter)->reassign_qloc(count);

	    Label node1 = (*iter)->get_node1()->get_node_id();	
	    Label node2 = (*iter)->get_node2()->get_node_id();	

            QE tmpelem(val, std::make_pair(node1, node2));	
	    all_edges.push_back(tmpelem); 
	
  	    count++;
        }
    }

    MergePriorityQueue<QE> *Q = new MergePriorityQueue<QE>(rag);
    Q->set_storage(&all_edges);	
    PriorityQCombine node_combine_alg(feature_mgr, rag, Q); 

    clock_t start = clock();

    while (!Q->is_empty()){
	QE tmpqe = Q->heap_extract_min();	


	Label node1 = tmpqe.get_val().first;
	Label node2 = tmpqe.get_val().second;
        RagEdge_uit* rag_edge = rag->find_rag_edge(node1, node2);


        if (!rag_edge || !tmpqe.valid()) {
            continue;
        }


        RagNode_uit* rag_node1 = rag_edge->get_node1();
        RagNode_uit* rag_node2 = rag_edge->get_node2();

        node1 = rag_node1->get_node_id(); 
        node2 = rag_node2->get_node_id(); 


        edge_label = decide_edge_label(rag_node1, rag_node2);

	unsigned long long node1sz = rag_node1->get_size();	
	unsigned long long node2sz = rag_node2->get_size();	



	if  (edge_label){  
	    std::vector<double> feature;
	    feature_mgr->compute_all_features(rag_edge,feature);


	    feature.push_back(edge_label);	
            all_featuresu.insert(feature);
	}	
        else 
	    int checkp = 1;	

	if ( edge_label == -1 ){ //merge
	    modify_assignment_after_merge(node1, node2);  	

            rag_join_nodes(*rag, rag_node1, rag_node2, &node_combine_alg);  
	    watershed_to_body[node2] = node1;
	    for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
		watershed_to_body[*iter] = node1;
	    }
	    merge_history[node1].push_back(node2);
	    merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
	    merge_history.erase(node2);
	}

    }
    
    std::vector< std::vector<double> >	all_features;
    all_featuresu.get_feature_label(all_features, all_labels);	
    
    //printf("Time required to learn RF: %.2f with oob :%f\n", , oob_v.oob_breiman);
    printf("Features generated in %.2fsecs\n",((double)clock() - start) / CLOCKS_PER_SEC);
    feature_mgr->get_classifier()->learn(all_features, all_labels); // number of trees
    printf("Classifier learned \n");

    /*debug*/
    double err=0;
    for(int fcount=0;fcount<all_features.size(); fcount++){
	double predp = feature_mgr->get_classifier()->predict(all_features[fcount]);
	int predl = (predp>0.5)? 1:-1;	
	err+= ((predl==all_labels[fcount])?0:1);	
    }
    printf("accuracy = %.3f\n",100*(1 - err/all_labels.size()));	
    /**/

    if(clfr_filename){
	feature_mgr->get_classifier()->save_classifier(clfr_filename);
	printf("Classifier saved to %s\n",clfr_filename);
    }

}


