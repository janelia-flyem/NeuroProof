#include "Stack.h"

#include "../Rag/Properties/MitoTypeProperty.h"

using namespace NeuroProof;
using namespace std;
using namespace vigra;

// *******************************************merge cyto********************************************************************


void StackPredict::agglomerate_rag_mrf(double threshold, bool read_off, string output_path, string classifier_path)
{
    agglomerate_rag(0.06,false); //0.01 for 250, 0.02 for 500
    remove_inclusions();	  	
    printf("Remaining regions: %d\n",rag->get_num_regions());	

    unsigned int count=0;	
    unsigned int edgeCount=0;	
    std::vector< std::pair<Label, Label> > allEdges;	    	

    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {
	    double prev_val = (*iter)->get_weight();	
            double val = feature_mgr->get_prob(*iter);
	    if ((fabs(val-prev_val))>1e-7) 	
		count++;
            (*iter)->set_weight(val);

	    (*iter)->reassign_qloc(edgeCount);

	    Label node1 = (*iter)->get_node1()->get_node_id();	
	    Label node2 = (*iter)->get_node2()->get_node_id();	
	    allEdges.push_back(std::make_pair(node1, node2)); 

	    edgeCount++;
	}
    }

    unsigned found = classifier_path.find_last_of("/");
    string filename = classifier_path.substr(found+1);
    found = output_path.find_last_of("/");
    string dirname = output_path.substr(0,found);

    unsigned dot_found = filename.find_last_of(".");
    string wts_fname = filename.substr(0,dot_found);	
    string wts_path = dirname+ "/"+wts_fname;	

    bool file_exists = false;
    string wts_path_i;
    string prob_path_i;
    
    if (read_off)  { 
        for (int bp_iterCount=0; bp_iterCount < 5; bp_iterCount++){ 	
	    char itr_str[256];
	    
	    sprintf(itr_str,"%d",bp_iterCount+1);
	    string wts_path2 = wts_path + "_rwts" + itr_str + ".txt";    		
	    string prob_path2 = wts_path + "_analysis_prob"+ itr_str +".txt";    		
	    FILE* fps = fopen(wts_path2.c_str(),"rt");    	
	    if (fps){
		file_exists = true;
		wts_path_i = wts_path2;
		prob_path_i = prob_path2;
		fclose(fps);
	    }
	    else
		break;
	}
    }

    if(read_off && file_exists){	
	//int bp_iterCount = 4;
	//char itr_str[256];
	//sprintf(itr_str,"%d",final_iterCount);
	//string wts_path_i = wts_path + "_rwts" + itr_str + ".txt";    		
	printf("reading weights from %s\n", wts_path_i.c_str());
        BatchMergeMRFh mergeMRF(rag,feature_mgr,&assignment,threshold);	
	std::vector<double> wts;
	
	
	mergeMRF.read_and_set_tree_weights(wts_path_i, wts);	
	mergeMRF.refine_edge_weights(allEdges, prob_path_i);
    }	
    else{  	
        compute_groundtruth_assignment();
        for (int bp_iterCount=0; bp_iterCount < 5; bp_iterCount++){ 	
     	   BatchMergeMRFh mergeMRF(rag,feature_mgr,&assignment,threshold);	
	   
	   char itr_str[256];
	   sprintf(itr_str,"%d", bp_iterCount+1);
	   string wts_path_i = wts_path + "_rwts" + itr_str + ".txt";    		
	   string prob_path_i = wts_path + "_analysis_prob"+ itr_str +".txt";    		
     	   double maxdiff= mergeMRF.compute_merge_prob( bp_iterCount, allEdges, wts_path_i, prob_path_i);	
	   if (maxdiff<0.01)
	       break;

    	}
    }



    agglomerate_rag(threshold,true);
    
}

void StackPredict::agglomerate_rag_queue(double threshold, bool use_edge_weight, string output_path, string classifier_path)
{
    
    std::vector<QE> all_edges;	    	

    int count=0; 	
    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {
	  

	    RagNode<Label>* rag_node1 = (*iter)->get_node1();
	    RagNode<Label>* rag_node2 = (*iter)->get_node2();
	    
	    Label node1 = rag_node1->get_node_id(); 
	    Label node2 = rag_node2->get_node_id(); 
	  
	    double val;
	    if(use_edge_weight)
            	val = (*iter)->get_weight();
	    else	
		val = feature_mgr->get_prob(*iter);    
	    
            (*iter)->set_weight(val);
	    (*iter)->reassign_qloc(count);

            QE tmpelem(val,std::make_pair(node1,node2));	
	    all_edges.push_back(tmpelem); 
	
  	    count++;
        }
    }
    
    double error=0;  	

    MergePriorityQueue<QE> *Q = new MergePriorityQueue<QE>(rag);
    Q->set_storage(&all_edges);	
	
    while (!Q->is_empty()){
	QE tmpqe = Q->heap_extract_min();	


        //RagEdge<Label>* rag_edge = tmpqe.get_val();
        Label node1 = tmpqe.get_val().first;
        Label node2 = tmpqe.get_val().second;
        RagEdge<Label>* rag_edge = rag->find_rag_edge(node1,node2);;

        if (!rag_edge || !tmpqe.valid()) {
            continue;
        }
	double prob = tmpqe.get_key();
	if (prob>threshold)
	    break;	

        RagNode<Label>* rag_node1 = rag_edge->get_node1();
        RagNode<Label>* rag_node2 = rag_edge->get_node2();
        node1 = rag_node1->get_node_id(); 
        node2 = rag_node2->get_node_id(); 

	try {    
            MitoTypeProperty& mtype1 = rag_node1->get_property<MitoTypeProperty>("mito-type");
            MitoTypeProperty& mtype2 = rag_node2->get_property<MitoTypeProperty>("mito-type");
            if ((mtype1.get_node_type()==2) || (mtype2.get_node_type()==2)) {	
                continue;
            }
        } catch (ErrMsg& msg) {

        }

	if(gtruth)
	    error += ((assignment.find(node1)->second == assignment.find(node2)->second) ? 0 : 1);
	    
	    
        rag_merge_edge_priorityq(*rag, rag_edge, rag_node1, Q, feature_mgr);
        watershed_to_body[node2] = node1;
        for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
            watershed_to_body[*iter] = node1;
        }
        merge_history[node1].push_back(node2);
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);


    }		
    
    if(gtruth)	
        printf("# false merges %.2f\n",error); 	
}

void StackPredict::agglomerate_rag_flat(double threshold, bool use_edge_weight, string output_path, string classifier_path)
{
   
    std::map<Label, Label> unique_node_id;
    std::map<Label, Label> unique_node_id_reverse;
    Label count = 0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
	Label id = (*iter)->get_node_id();
	if (unique_node_id.find(id) == unique_node_id.end()){
	    unique_node_id.insert(std::make_pair(id, count));
	    unique_node_id_reverse.insert(std::make_pair(count, id));
	    count++;
	}
    }
  
    unsigned found = output_path.find_last_of("/");
    string filename = output_path.substr(found+1);
    found = output_path.find_last_of("/");
    string dirname = output_path.substr(0,found);

    unsigned dot_found = filename.find_last_of(".");
    string wts_fname = filename.substr(0,dot_found);   
    
    wts_fname.replace(wts_fname.end()-11,wts_fname.end()-3,"" );
    
    string wts_path = dirname+ "/"+ wts_fname + "_edges.txt";	
    string wts_path2 = dirname+ "/"+ wts_fname + "_prob.txt";	
    
    //*C* to read
    char tmpstr[255];
    sprintf(tmpstr,"%.2lf",threshold);   
    string wts_path3 = dirname+ "/"+ wts_fname + "_prob_beta"+ tmpstr + "_mi50.txt";	
    
    
    FILE* initial_fid_edge = fopen(wts_path.c_str(),"rt");
    FILE* fid_label = fopen(wts_path3.c_str(),"rt");
    
    std::vector<QE> priority;	    	
    
    Label a,b;
    Label lbl;
    while(!feof(initial_fid_edge)){
	fscanf(initial_fid_edge,"%u %u\n",&a,&b);
	fscanf(fid_label, "%u\n",&lbl);
	if (lbl==0){
	    Label node1 = unique_node_id_reverse[a];
	    Label node2 = unique_node_id_reverse[b];
	    QE tmpelem(0, std::make_pair(node1, node2 ));	
	    priority.push_back(tmpelem); 
	}
      
    }
    fclose(initial_fid_edge);
    fclose(fid_label);
    
    int ii=0;	
    for(ii=0; ii< priority.size(); ii++) {
	
	QE tmpqe = priority[ii];	
        Label node1 = tmpqe.get_val().first;
        Label node2 = tmpqe.get_val().second;
	if(node1==node2)
	    continue;
	
        RagEdge<Label>* rag_edge = rag->find_rag_edge(node1,node2);;

        if (!rag_edge || !(priority[ii].valid()) || (rag_edge->get_weight())>threshold ) {
            continue;
        }

        RagNode<Label>* rag_node1 = rag_edge->get_node1();
        RagNode<Label>* rag_node2 = rag_edge->get_node2();
	try {    
            MitoTypeProperty& mtype1 = rag_node1->get_property<MitoTypeProperty>("mito-type");
            MitoTypeProperty& mtype2 = rag_node2->get_property<MitoTypeProperty>("mito-type");
            if ((mtype1.get_node_type()==2) || (mtype2.get_node_type()==2)) {	
                continue;
            }
        } catch (ErrMsg& msg) {

        }

        node1 = rag_node1->get_node_id(); 
        node2 = rag_node2->get_node_id(); 
	//printf("node keep: %d, node remove :%d\n",node1,node2);
        rag_merge_edge_flat(*rag, rag_edge, rag_node1, priority, feature_mgr);
        watershed_to_body[node2] = node1;
        for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
            watershed_to_body[*iter] = node1;
        }
        merge_history[node1].push_back(node2);
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);
    }
    
}



void StackPredict::agglomerate_rag(double threshold, bool use_edge_weight, string output_path, string classifier_path, bool synapse_mode)
{
  
    double error=0;  	
    
    MergePriority* priority = new ProbPriority(feature_mgr, rag, synapse_mode);
    priority->initialize_priority(threshold, use_edge_weight);

    while (!(priority->empty())) {

        RagEdge<Label>* rag_edge = priority->get_top_edge();

        if (!rag_edge) {
            continue;
        }

        RagNode<Label>* rag_node1 = rag_edge->get_node1();
        RagNode<Label>* rag_node2 = rag_edge->get_node2();

	try {    
            MitoTypeProperty& mtype1 = rag_node1->get_property<MitoTypeProperty>("mito-type");
            MitoTypeProperty& mtype2 = rag_node2->get_property<MitoTypeProperty>("mito-type");
            if ((mtype1.get_node_type()==2) || (mtype2.get_node_type()==2)) {	
                continue;
            }
        } catch (ErrMsg& msg) {

        }

        Label node1 = rag_node1->get_node_id(); 
        Label node2 = rag_node2->get_node_id();

	if(gtruth)
	    error += ((assignment.find(node1)->second == assignment.find(node2)->second) ? 0 : 1);

        rag_merge_edge_median(*rag, rag_edge, rag_node1, priority, feature_mgr);
        watershed_to_body[node2] = node1;
        for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
            watershed_to_body[*iter] = node1;
        }
        merge_history[node1].push_back(node2);
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);

	modify_assignment_after_merge(node1,node2);

    }
    //printf("# edges driven out %d\n",priority->get_kout()); 	
    if(gtruth)	
        printf("# false merges %.2f\n",error); 	

}

void StackPredict::agglomerate_rag_size(double threshold)
{}



// ****************************************Merge mito***************************************************************


void StackPredict::merge_mitochondria_a()
{
    double error=0;  	

    MergePriority* priority = new MitoPriority(feature_mgr, rag);
    priority->initialize_priority(0.8);

    while (!(priority->empty())) {

        RagEdge<Label>* rag_edge = priority->get_top_edge();

        if (!rag_edge) {
            continue;
        }

        RagNode<Label>* rag_node1 = rag_edge->get_node1();
        RagNode<Label>* rag_node2 = rag_edge->get_node2();

        try {
            MitoTypeProperty& mtype1 = rag_node1->get_property<MitoTypeProperty>("mito-type");
            MitoTypeProperty& mtype2 = rag_node2->get_property<MitoTypeProperty>("mito-type");
            if ((mtype1.get_node_type()==2) && (mtype2.get_node_type()==1))	{
                RagNode<Label>* tmp = rag_node1;
                rag_node1 = rag_node2;
                rag_node2 = tmp;		
            } else if ((mtype2.get_node_type()==2) && (mtype1.get_node_type()==1))	{
                // nophing	
            } else {
                continue;
            }
        } catch (ErrMsg& msg) {
            continue;
        }

        Label node1 = rag_node1->get_node_id(); 
        Label node2 = rag_node2->get_node_id();

	if(gtruth)
	    error += ((assignment.find(node1)->second == assignment.find(node2)->second) ? 0 : 1);

        rag_merge_edge_median(*rag, rag_edge, rag_node1, priority, feature_mgr);
        watershed_to_body[node2] = node1;
        for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
            watershed_to_body[*iter] = node1;
        }
        merge_history[node1].push_back(node2);
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);

	modify_assignment_after_merge(node1,node2);

    }

    //printf("# edges driven out %d\n",priority->get_kout()); 	
    if(gtruth)	
        printf("# false merges %.2f\n",error); 	

    unsigned long not_merged_total=0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label id = (*iter)->get_node_id();

        try {        
            MitoTypeProperty& mtype = (*iter)->get_property<MitoTypeProperty>("mito-type");
            if(mtype.get_node_type()==2)
                not_merged_total++;
        } catch (ErrMsg& msg) {

        }
    }
    //printf("Total mito not merged %d\n", not_merged_total);	
}




// ***********************************absorb small regions*******************************************************


void StackPredict::absorb_small_regions(double* prediction_vol, Label* label_vol){

    std::set<Label> small_regions;
     
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        unsigned long long  sz = (*iter)->get_size();
	
	if(sz<100)
	    small_regions.insert((*iter)->get_node_id());
    }

    unsigned long volsz = (depth-2*padding)*(height-2*padding)*(width-2*padding);

    unsigned long nnz=0; 	
    for(unsigned long i=0; i< volsz; i++){
	Label lbl = label_vol[i];
	if (small_regions.find(lbl) != small_regions.end()){
	    label_vol[i] = 0;	
	    nnz++;	
	}
    } 

    VigraWatershed wshed(depth-2*padding,height-2*padding,width-2*padding);
    wshed.run_watershed(prediction_vol,label_vol);

    
}

int StackPredict::absorb_small_regions2(double* prediction_vol, Label* label_vol, int threshold){

    std::tr1::unordered_map<Label, unsigned long long> regions_sz;

    unsigned long volsz = (depth-2*padding)*(height-2*padding)*(width-2*padding);
    std::tr1::unordered_map<Label, unsigned long long>::iterator it;
    for(unsigned long long i=0; i< volsz; i++){
	Label lbl = label_vol[i];
	it = regions_sz.find(lbl);
	if ( it != regions_sz.end()){
	    (it->second)++;    
	}
	else{
	    regions_sz.insert(std::make_pair(lbl,1));
	}
    } 
    
    std::tr1::unordered_map<Label, int> synapse_counts;
    load_synapse_counts(synapse_counts);
    int num_removed = 0;    
    std::tr1::unordered_set<Label> small_regions;
    for(it = regions_sz.begin(); it != regions_sz.end(); it++) {
	if ((synapse_counts.find(it->first) == synapse_counts.end()) &&
                ((it->second) < threshold)) {
	    small_regions.insert(it->first);
	    ++num_removed;	
        }
    }

    for(unsigned long i=0; i< volsz; i++){
	Label lbl = label_vol[i];
	if (small_regions.find(lbl) != small_regions.end()){
	    label_vol[i] = 0;	
	}
    } 
    //printf("total zeros: %d\n",nnz);
    VigraWatershed wshed(depth-2*padding,height-2*padding,width-2*padding);
    wshed.run_watershed(prediction_vol,label_vol);

    return num_removed;
}

