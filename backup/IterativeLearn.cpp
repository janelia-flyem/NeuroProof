

#include "IterativeLearn.h"

using namespace NeuroProof;


void IterativeLearn::update_clfr_name(string &clfr_name, size_t trnsz)
{
  
    size_t found = clfr_name.find("trnsz");
    size_t end = clfr_name.find("_",found+1);
    string newstr = "trnsz"; 
    char szstr[255];
    sprintf(szstr,"%d",trnsz);//(trnsz, szstr,10);
    newstr += szstr;
    
    clfr_name.replace(clfr_name.begin()+found,clfr_name.begin()+end, newstr);
}








void IterativeLearn::edgelist_from_index(std::vector<unsigned int>& new_idx,
					      std::vector< std::pair<Node_t, Node_t> >& elist)
{
  
    elist.clear();
    elist.resize(new_idx.size());
    for(size_t ii=0; ii < new_idx.size(); ii++){
	unsigned int idx = new_idx[ii];
	elist[ii] = edgelist[idx];
    }
}





// **************************************************************************************************

void IterativeLearn::compute_all_edge_features(std::vector< std::vector<double> >& all_features,
					      std::vector<int>& all_labels){

    
    int count=0; 	
    for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {

        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {
	    RagEdge_t* rag_edge = *iter; 	

            RagNode_t* rag_node1 = rag_edge->get_node1();
            RagNode_t* rag_node2 = rag_edge->get_node2();
            Node_t node1 = rag_node1->get_node_id(); 
            Node_t node2 = rag_node2->get_node_id(); 

// 	    int edge_label = stack->decide_edge_label(rag_node1, rag_node2);
	    int edge_label = 2;
	    if (stack->get_gt_labelvol() != NULL){
		printf("Full groundtruth CANNOT be provided in this mode, try other versions\n");
		exit(0);
	    }
	    else if (use_mito){
		if ((stack->is_mito(node1) && 
				stack->is_mito(node2))) {
		    edge_label = 0; 
		}
		else if ((stack->is_mito(node1) || 
				stack->is_mito(node2))) {
		    edge_label = 1; 
		}
	    }
	
 	    unsigned long long node1sz = rag_node1->get_size();	
	    unsigned long long node2sz = rag_node2->get_size();	

	    if ( edge_label ){	
		std::vector<double> feature;
		feature_mgr->compute_all_features(rag_edge,feature);

		all_features.push_back(feature);
		all_labels.push_back(edge_label);

		edgelist.push_back(std::make_pair(node1,node2));
// 		feature.push_back(edge_label);
// 		all_featuresu.insert(feature);

	    }	
	    else 
		int checkp = 1;	

        }
    }
    
    /*C* Debug
    all_features.erase(all_features.begin()+1000, all_features.end());
    all_labels.erase(all_labels.begin()+1000, all_labels.end());
    //**/

}



void IterativeLearn::evaluate_accuracy(std::vector< std::vector<double> >& test_features,
			       std::vector<int>& test_labels, double thd )
{
    size_t nexamples2tst = test_features.size();
    size_t nexamples_p = 0;
    size_t nexamples_n = 0;
    
    
    
    double corr_p=0, corr_n=0, fp=0, fn= 0 ;
    for(int ecount=0;ecount< test_features.size(); ecount++){
	double predp = feature_mgr->get_classifier()->predict(test_features[ecount]);
	int predl = (predp > thd)? 1:-1;	
	int actuall = test_labels[ecount];
	
	actuall==1 ? nexamples_p++ : nexamples_n++;
	
	corr_p += ((predl== 1 && actuall ==1) ? 1: 0);
	corr_n += ((predl== -1 &&  actuall ==-1) ? 1: 0);
	fp += ((predl== 1 &&  actuall == -1) ? 1: 0);
	fn += ((predl== -1 &&  actuall == 1) ? 1: 0);
	
	//err+= ((predl== subset_labels[ecount])?0:1);	
    }
    //printf("accuracy = %.3f\n",100*(1 - err/subset_labels.size()));	
    printf("total tst samples= %u\n",nexamples2tst);
    printf("correct p = %.1f\n", corr_p*100.0/nexamples2tst);
    printf("correct n = %.1f\n", corr_n*100.0/nexamples2tst);
    printf("false p = %.1f\n", fp*100.0/nexamples2tst);
    printf("false n = %.1f\n", fn*100.0/nexamples2tst);

}
void IterativeLearn::evaluate_accuracy(std::vector<int>& labels, std::vector<double>& predicted_vals, double thd)
{
    size_t nexamples2tst = labels.size();
    size_t nexamples_p = 0;
    size_t nexamples_n = 0;
    
//     double thd = 0.3 ;
    
    double corr_p=0, corr_n=0, fp=0, fn= 0, undecided=0 ;
    for(int ecount=0;ecount< labels.size(); ecount++){
	double predp = predicted_vals[ecount];
	int predl = (predp > thd)? 1:-1;
	if (!(predp > thd) &&  !(predp<thd)){
	  predl=0;
	  undecided++;
	}
	int actuall = labels[ecount];
	
	actuall==1 ? nexamples_p++ : nexamples_n++;
	
	corr_p += ((predl== 1 && actuall ==1) ? 1: 0);
	corr_n += ((predl== -1 &&  actuall ==-1) ? 1: 0);
	fp += ((predl== 1 &&  actuall == -1) ? 1: 0);
	fn += ((predl== -1 &&  actuall == 1) ? 1: 0);
	
	//err+= ((predl== subset_labels[ecount])?0:1);	
    }
    //printf("accuracy = %.3f\n",100*(1 - err/subset_labels.size()));	
    printf("total tst samples= %u\n",nexamples2tst);
    printf("correct p = %.1f\n", corr_p*100.0/nexamples2tst);
    printf("correct n = %.1f\n", corr_n*100.0/nexamples2tst);
    printf("false p = %.1f\n", fp*100.0/nexamples2tst);
    printf("false n = %.1f\n", fn*100.0/nexamples2tst);
    printf("undecided samples= %.2f\n",undecided*100.0/nexamples2tst);

}


