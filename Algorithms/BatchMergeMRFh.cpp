#include "BatchMergeMRFh.h"
#include "../BioPriors/MitoTypeProperty.h"

#include <ctime>
#include <cstdlib>
#include <cmath>

// #include <dai/alldai.h>  // Include main libDAI header file
// #include <dai/bp.h>  // Include main libDAI header file
// #include <dai/util.h>  // Include main libDAI header file

// using namespace dai;
using namespace std;

#define MERGE_COST(p,th) (p>th)?1:p
#define KEEP_COST(p,th) (p>th)?0:1-p
#define C_EPS 0.001


BatchMergeMRFh::BatchMergeMRFh(Rag_t* prag, FeatureMgr* pfmgr, multimap<Node_t, Node_t>* assignment, double pthd, size_t psz): _rag(prag), _feature_mgr(pfmgr), _subsetSz(psz), _thd(pthd) {

    _assignment = assignment;
    if (_subsetSz==2){
        _labelConfig.resize(2*_subsetSz);
        _labelConfig[0].resize(_subsetSz); _labelConfig[0][0]=MERGE; _labelConfig[0][1] = MERGE;	
        _labelConfig[1].resize(_subsetSz); _labelConfig[1][0]=KEEP; _labelConfig[1][1] = MERGE;	
        _labelConfig[2].resize(_subsetSz); _labelConfig[2][0]=MERGE; _labelConfig[2][1] = KEEP;	
        _labelConfig[3].resize(_subsetSz); _labelConfig[3][0]=KEEP; _labelConfig[3][1] = KEEP;	
    }

    _srag = new Rag_t();
    _sfeature_mgr = new FeatureMgr();
    _sfeature_mgr->copy_channel_features(_feature_mgr);
    _sfeature_mgr->set_classifier(_feature_mgr->get_classifier());
    for(int i=2; i<= _subsetSz; i++){
        vector< vector<int> > allConfig;
        ComputeTempIndex(allConfig, 2, i);	
        _configList.insert(make_pair(i, allConfig));	
    }
};



double BatchMergeMRFh::compute_merge_prob( int iterCount, std::vector< std::pair<Node_t, Node_t> >& allEdges, 
					   string wts_path, string analysis_path){
	


    _update_belief = 0.1;	
    _bthd = 0.3;	

    time_t start, end;
    time(&start);
//    _allEdges.clear();
//    _edgeWt.clear();		
	
    unsigned int edgeCount = allEdges.size(); 	

    double maxDiff = 0;

/*    for (Rag_t::edges_iterator iter = _rag->edges_begin(); iter != _rag->edges_end(); ++iter) {
        if ( (!(*iter)->is_preserve()) && (!(*iter)->is_false_edge()) ) {
            double val = _feature_mgr->get_prob(*iter);
            //(*iter)->set_weight(val);
	    (*iter)->reassign_qloc(_edgeCount);

	    Label node1 = (*iter)->get_node1()->get_node_id();	
	    Label node2 = (*iter)->get_node2()->get_node_id();	
	    _allEdges.push_back(std::make_pair(node1, node2)); 
	
	    _edgeWt.push_back(0);	
  	    _edgeCount++;
        }
	else
	    int checkp=1;	
    }*/

    int prev_edges = _rag->get_num_edges();
    _edgeBlf.resize(edgeCount);	

    /*FILE* fp=fopen("node_neighbors.txt","wt");	
    for (Rag_t::nodes_iterator iter = _rag->nodes_begin(); iter != _rag->nodes_end(); ++iter) 
	if (*iter)
	    fprintf(fp,"%d %d\n",(*iter)->get_size(),(*iter)->node_degree());

    fclose(fp);	*/



    for (Rag_t::nodes_iterator iter = _rag->nodes_begin(); iter != _rag->nodes_end(); ++iter) 
        
        if (*iter) {
            try {
                MitoTypeProperty& mtype = (*iter)->get_property<MitoTypeProperty>("mito-type");
                if (mtype.get_node_type() != 2) {
                    generate_subsets(*iter);
                }
            } catch (ErrMsg& msg) {
                generate_subsets(*iter);
            }
        }

    /*fprintf(_fp,"\n\n"); 	
    for (int i=0;i<_subsets.size();i++){
	for(int j=0; j< _subsets[i].size();j++)	
	    fprintf(_fp,"%d ",_subsets[i][j]);	
	fprintf(_fp,"\n");
   }*/

    string rsp_fname("wk_responses.txt");	
    FILE* fpr = fopen(rsp_fname.c_str(), "wt");	

    int changeCount=0;	
    vector<double> newprobs(edgeCount);	
    for(int i=0; i < edgeCount; i++){
	Node_t node1 = allEdges[i].first;
	Node_t node2 = allEdges[i].second;

	RagEdge_t* edge1 = _rag->find_rag_edge(node1, node2);;

	double prob = edge1->get_weight();
	
	double newprob = prob;
	if (prob==0)
	    int checkp=1;	


	if(_edgeBlf[i].size()<2){
	    changeCount++;
	    newprobs[i] = newprob; 	
	    continue;
	}
	double vote0 = _edgeBlf[i][0];
	double vote1 = _edgeBlf[i][1];
	double sumvote = vote0+vote1;	
	vote0 /= sumvote;
	vote1 /= sumvote;


	if ((vote1-vote0)>0)
	    newprob = prob *(1+ ((vote1-vote0)/2));
	else
	    newprob = prob * (1+ ((vote1-vote0)/4));

	newprob = (newprob>1.0)? 1.0:newprob;
        newprobs[i] = newprob;

	vector<double> wk_responses;
	_feature_mgr->get_responses(edge1, wk_responses);
	for(int rr=0; rr < wk_responses.size(); rr++){
	    fprintf(fpr, "%.5f ", wk_responses[rr]);
	}
	fprintf(fpr, "%.5f\n",newprob);
    } 	
    fclose(fpr);	
	

    system("matlab-solve-qp/run_solve_qp_matlab.sh  matlab-solve-qp/MCR/mcr-install/v717  wk_responses.txt");

    string sol_fname = rsp_fname;
    sol_fname.replace(sol_fname.end()-4, sol_fname.end(),"_sol.txt"); 		
    vector<double> tree_wts;

    read_and_set_tree_weights(sol_fname, tree_wts);	


    maxDiff = refine_edge_weights(allEdges, analysis_path);	

    printf("maxdiff = %f\n", maxDiff);
    printf("no changes: %d\n",changeCount);	

    //write_in_file("tmp_pairs.txt");	
    //run_inference(); 		

    time(&end);	
    printf("Time elapsed: %.2f\n", (difftime(end,start))*1.0/60);
    
  
    int tt=1;
    
    string syscommand = "mv -f ";
    syscommand = syscommand + sol_fname+ " " ; 	 	
    syscommand = syscommand + wts_path ;    	

    system(syscommand.c_str()); 	

    return maxDiff;	
}

void BatchMergeMRFh::read_and_set_tree_weights(string sol_fname, vector<double>& tree_wts)
{
    FILE* fps = fopen(sol_fname.c_str(),"rt");	
    while(!feof(fps)){
	float coeff;	
	fscanf(fps,"%f\n", &coeff);
	tree_wts.push_back(coeff);
    }
    fclose(fps);

    _feature_mgr->get_classifier()->set_tree_weights(tree_wts);

}

double BatchMergeMRFh::refine_edge_weights(std::vector< std::pair<Node_t, Node_t> >& allEdges, string tmp_fname)
{
    double maxDiff = 0;	
    unsigned int edgeCount = allEdges.size(); 	

    //string tmp_fname("analysis/tmp_config");
    //char itst[10];
    //sprintf(itst,"%d",iterCount);	 	
    // tmp_fname += itst;	 	
    //tmp_fname += ".txt";

//    if(iterCount>=0)  	
    _fp = fopen(tmp_fname.c_str(),"wt");	

    for(int i=0; i < edgeCount; i++){
	Node_t node1 = allEdges[i].first;
	Node_t node2 = allEdges[i].second;

	RagEdge_t* edge1 = _rag->find_rag_edge(node1, node2);;

	int edge_label = get_gt(edge1);

	double prob = edge1->get_weight();

	double newresp = _feature_mgr->get_prob(edge1);	

	maxDiff = maxDiff < fabs(newresp-prob)? fabs(newresp-prob) : maxDiff;

	//fprintf(_fp, "%d  %.3f  %.3f %.3f\n", edge_label, prob, newprobs[i], newresp);
	//if (iterCount>=0)
	fprintf(_fp, "%d  %.3f  %.3f\n", edge_label, prob,  newresp);

        edge1->set_weight(newresp);

	//_allEdges[i]= ;
	//_edgeWt[i] = 0.0;
    }

    //if (iterCount>=0)
    fclose(_fp);	
    return maxDiff;	
}

void BatchMergeMRFh::generate_subsets(RagNode_t* pnode)
{
	
    set<Node_t> nbr_set;
    multimap<Node_t, Node_t> nbr_set_degree;	
    for(RagNode_t::edge_iterator iter = pnode->edge_begin(); iter != pnode->edge_end(); ++iter) {
	RagNode_t* other_node = (*iter)->get_other_node(pnode);

        try {
            MitoTypeProperty& mtype = other_node->get_property<MitoTypeProperty>("mito-type");
            if (mtype.get_node_type()!=2) {
                nbr_set.insert(other_node->get_node_id());
            }
        } catch (ErrMsg& msg) {
            nbr_set.insert(other_node->get_node_id());
        }

	//fprintf(_fp,"edge: (%d, %d), loc %d\n",pnode->get_node_id(), other_node->get_node_id(), (*iter)->get_qloc());
    }
    if (nbr_set.size()<=1)
	return; 	
    //fprintf(_fp,"\n");	
    for(set<Node_t>::iterator it=nbr_set.begin(); it!= nbr_set.end(); it++){
 	Node_t nbr1 = *it;
	int sdegree=0;
	RagNode_t* rag_nbr1 = _rag->find_rag_node(nbr1);
	int degree1 = rag_nbr1->node_degree();
	for(RagNode_t::edge_iterator iter2 = rag_nbr1->edge_begin(); iter2 != rag_nbr1->edge_end(); ++iter2){
	    Node_t nbr_to_nbr1 = (*iter2)->get_other_node(rag_nbr1)->get_node_id();
	    if (nbr_set.find(nbr_to_nbr1)!= nbr_set.end())
		sdegree++;	
	}
	nbr_set_degree.insert(make_pair(sdegree, rag_nbr1->get_node_id()));
    }	

    set<Node_t> nbr_set2 = nbr_set;
    set<Node_t> subset;

    Node_t prev_node = pnode->get_node_id();

    while(!nbr_set_degree.empty()){
	multimap<Node_t, Node_t>::reverse_iterator nsdb;
	for(nsdb = nbr_set_degree.rbegin(); nsdb!= nbr_set_degree.rend(); nsdb++){
	    RagNode_t* rag_node1 = _rag->find_rag_node((*nsdb).second);
	    RagEdge_t* edge1= _rag->find_rag_edge(rag_node1, _rag->find_rag_node(prev_node));
	    if (edge1)
		break;	
	}
	if (nsdb== nbr_set_degree.rend())
	    nsdb = nbr_set_degree.rbegin();	
	
	Node_t node1= (*nsdb).second;
	subset.insert(node1);

	prev_node = node1;

	nbr_set_degree.erase(--nsdb.base());		
	nbr_set2.erase(node1);

	if (subset.size()>= _subsetSz){
	    compute_subset_cost(pnode, subset);
	    subset.clear();		
	}
    }
    if (subset.size()>0){
	if (nbr_set.size()> _subsetSz){
	    for(set<Node_t>::iterator it=nbr_set.begin();it!=nbr_set.end(); it++){	
		Node_t nbr1=*it;
		subset.insert(nbr1);
		if (subset.size() == _subsetSz)
		    break;
	    }	
	}
	compute_subset_cost(pnode, subset);
	subset.clear();		
    }			
}
void BatchMergeMRFh::compute_subset_cost(RagNode_t* pnode, set<Node_t>& subset)
{

    //vector< vector<int> > allConfig;
    //ComputeTempIndex(allConfig,2,subset.size());	

    if (subset.size() > _subsetSz)
	int pp=1;	

    multimap<int, vector< vector<int> > >::iterator citer; 	
    citer = _configList.find(subset.size()); 	
    vector< vector<int> >& allConfig = (*citer).second;

    /*for(i=0;i<allConfig.size();i++){
	for(j=0;j<allConfig[i].size();j++)
	    printf("%d ",allConfig[i][j]);
	printf("\n"); 		
    }*/

    build_srag(pnode,subset);
    vector<Node_t> gts;	     	
    Node_t node_id = pnode->get_node_id();
    for(set<Node_t>::iterator it= subset.begin(); it != subset.end(); it++){
	Node_t nbr_id = (*it);
	RagEdge_t* srag_edge = _srag->find_rag_edge(node_id, nbr_id);
	
	int elbl = get_gt(srag_edge);
	gts.push_back(elbl);
    }	 	

    vector<double> cost1;
    multimap<double, vector<int> > tmpcost;		


    vector<Node_t> subset1;
    vector<Node_t> edge_idx;

    vector<double> edge_prob;	

    for(set<Node_t>::iterator it= subset.begin(); it!=subset.end(); it++){
	subset1.push_back((*it));
	RagEdge_t* edge1 = _rag->find_rag_edge(pnode->get_node_id(),(*it));
	
        int qloc = -1;
        try {
            qloc = edge1->get_property<int>("qloc");
        } catch (ErrMsg& msg) {
        }

        edge_idx.push_back(qloc);
	edge_prob.push_back(edge1->get_weight());
    }	
    subset1.push_back(pnode->get_node_id());
      	

    _subsets.push_back(edge_idx);	

			

    for(int i=0;i< allConfig.size();i++){	

	//Rag_t* srag_copy = copy_srag(_srag);

	vector<int> config1 = allConfig[i];
	vector<int> merge_idx;
/*	for(int j=0; j< config1.size(); j++)
	    if (!config1[j])	
		merge_idx.push_back(j);*/

	multimap<double, int> sortedp;
	for(int j=0; j< config1.size(); j++)
	    if (!config1[j])	
		sortedp.insert(make_pair(edge_prob[j],j));

	for(multimap<double, int>::iterator iter=sortedp.begin(); iter!=sortedp.end();iter++){
	    int jj = (*iter).second;
	    merge_idx.push_back(jj); 	
	}

	double mincc = 1e6;
	int permcount=0;
	//do{
	     build_srag(pnode,subset);
	     double cc = merge_by_order(config1,subset1, merge_idx);
	     
 	     mincc = (mincc>cc) ? cc: mincc;	 	
	     permcount++;	
	//}while (next_permutation(merge_idx.begin(), merge_idx.end()));


	cost1.push_back(exp(-2.*mincc));
	
	tmpcost.insert(make_pair(cc,config1));

    }	


    std::vector< std::vector<double> > subset_to_edge;
    subset_to_edge.resize(_edgeBlf.size());
    
    for(multimap<double, vector<int> >::iterator iter= tmpcost.begin(); iter!=tmpcost.end(); iter++){
	vector<int> config1 = (*iter).second;
	double val = (*iter).first;
	val /= subset.size();

    	int count=0;	
    	for(set<Node_t>::iterator it= subset.begin(); it!=subset.end(); it++){
	    RagEdge_t* edge1 = _rag->find_rag_edge(pnode->get_node_id(),(*it));
	    
            int qloc = -1;
            try {
                qloc = edge1->get_property<int>("qloc");
            } catch (ErrMsg& msg) {
            }
	    
            if (subset_to_edge[qloc].size()==0)
		subset_to_edge[qloc].resize(2);
	    
	    (subset_to_edge[qloc][config1[count]]) += -log(val+C_EPS);	//dai::exp(-val) 	  	

	    count++;	
    	}	
    }
    
    for(set<Node_t>::iterator it= subset.begin(); it!=subset.end(); it++){
	RagEdge_t* edge1 = _rag->find_rag_edge(pnode->get_node_id(),(*it));
	
        int qloc = -1;
        try {
            qloc = edge1->get_property<int>("qloc");
        } catch (ErrMsg& msg) {
        }
	
        if (_edgeBlf[qloc].size()==0){
	    _edgeBlf[qloc].resize(2);
	    for(int ii=0; ii< _edgeBlf[qloc].size(); ii++)
		_edgeBlf[qloc][ii] = 1;
	}
	for(int ii=0; ii< _edgeBlf[qloc].size(); ii++)
	    _edgeBlf[qloc][ii] *= subset_to_edge[qloc][ii];
    }
    
    _costs.push_back(cost1);

}


double BatchMergeMRFh::merge_by_order(vector<int>& config, vector<Node_t>& subset, vector<int>& morder)
{
    double cost = 0;
    double thd = 1.0;	
    int node = subset[subset.size()-1];	
    RagNode_t* srag_node = _srag->find_rag_node(node);	
    for (int i=0 ; i < morder.size() ; i++){
	int idx = morder[i];
	int label = config[idx];	
	Node_t nbr = subset[idx];
	RagNode_t* srag_nbr = _srag->find_rag_node(nbr); 
	RagEdge_t* srag_edge = _srag->find_rag_edge(srag_node, srag_nbr);

//	if (label == MERGE){
	cost += MERGE_COST(srag_edge->get_weight(),thd);

        // PREVIOUS ERROR?: in the original the sizes would have been inreased twice?
	_sfeature_mgr->merge_features2(srag_node, srag_nbr,srag_edge);	
	srag_node->set_size(srag_node->get_size() + srag_nbr->get_size());	

	for (RagNode_t::edge_iterator it= srag_nbr->edge_begin(); it != srag_nbr->edge_end(); it++){
	    RagNode_t* other_node = (*it)->get_other_node(srag_nbr);
	    if (other_node == srag_node)
		continue;

	    RagEdge_t* temp_edge = _srag->find_rag_edge(srag_node, other_node);
	    if(temp_edge){ //merge features
		    //(*it)->print_edge();
		    //(temp_edge)->print_edge();	
                // PREVIOUS ERROR?: in the original the sizes would have been inreased twice?
		_sfeature_mgr->merge_features(temp_edge,(*it));	
		temp_edge->set_size(temp_edge->get_size() + (*it)->get_size());	
		double prob= _sfeature_mgr->get_prob(temp_edge); 	
		temp_edge->set_weight(prob);	
	    }	
	    else{ //copy features
		    //(*it)->print_edge();
		RagEdge_t* new_edge= _srag->insert_rag_edge(srag_node, other_node);	
		_sfeature_mgr->mv_features(new_edge,(*it));
		new_edge->set_weight((*it)->get_weight());
		new_edge->set_size((*it)->get_size());		 	
	    }
	    //}
	}
        _srag->remove_rag_node(srag_nbr);		
    }

    for (int i=0 ; i < config.size() ; i++){
	int label = config[i];
	if (label==0)
	    continue;		
	Node_t nbr = subset[i];
	RagNode_t* srag_nbr = _srag->find_rag_node(nbr); 
	RagEdge_t* srag_edge = _srag->find_rag_edge(srag_node, srag_nbr);
	
        cost += KEEP_COST(srag_edge->get_weight(),thd);	
    }		
    return cost;		
}



double BatchMergeMRFh::merge_by_config(vector<int>& config, vector<Node_t>& subset)
{
    double cost = 0;
    double thd = 1.0;	
    int node = subset[subset.size()-1];	
    RagNode_t* srag_node = _srag->find_rag_node(node);	
    for (int i=0 ; i < config.size() ; i++){
	int label = config[i];	
	Node_t nbr = subset[i];
	RagNode_t* srag_nbr = _srag->find_rag_node(nbr); 
	RagEdge_t* srag_edge = _srag->find_rag_edge(srag_node, srag_nbr);

	if (label == MERGE){
	    cost += MERGE_COST(srag_edge->get_weight(),thd);

            // PREVIOUS ERROR?: in the original the sizes would have been inreased twice?
	    _sfeature_mgr->merge_features(srag_node, srag_nbr);	
	    srag_node->set_size(srag_node->get_size() + srag_nbr->get_size());	

	    for (RagNode_t::edge_iterator it= srag_nbr->edge_begin(); it != srag_nbr->edge_end(); it++){
		RagNode_t* other_node = (*it)->get_other_node(srag_nbr);
		if (other_node == srag_node)
		    continue;

		RagEdge_t* temp_edge = _srag->find_rag_edge(srag_node, other_node);
		if(temp_edge){ //merge features
		    //(*it)->print_edge();
		    //(temp_edge)->print_edge();	
                    // PREVIOUS ERROR?: in the original the sizes would have been inreased twice?
		    _sfeature_mgr->merge_features(temp_edge,(*it));	
		    temp_edge->set_size(temp_edge->get_size() + (*it)->get_size());	
		    double prob= _sfeature_mgr->get_prob(temp_edge); 	
		    temp_edge->set_weight(prob);	
		}	
		else{ //copy features
		    //(*it)->print_edge();
		    RagEdge_t* new_edge= _srag->insert_rag_edge(srag_node, other_node);	
		    _sfeature_mgr->mv_features(new_edge,(*it));
		    new_edge->set_weight((*it)->get_weight());
		    new_edge->set_size((*it)->get_size());		 	
		}
	    }
	    _srag->remove_rag_node(srag_nbr);		
	}
	else{
	    cost += KEEP_COST(srag_edge->get_weight(),thd);	
	}
    }		
    return cost;		
}



void BatchMergeMRFh::build_srag(RagNode_t* pnode, set<Node_t>& subset)
{

    for (Rag_t::nodes_iterator iter = _srag->nodes_begin(); iter != _srag->nodes_end(); ++iter) 
	if (*iter)
	    _srag->remove_rag_node((*iter));

    NodeCaches &rag_node_cache= _feature_mgr->get_node_cache();  	
    EdgeCaches &rag_edge_cache= _feature_mgr->get_edge_cache();  	
    	
    RagNode_t* srag_common_node = _srag->insert_rag_node(pnode->get_node_id());
    _sfeature_mgr->copy_cache(rag_node_cache[pnode], srag_common_node);
    srag_common_node->set_boundary_size(pnode->get_boundary_size());	
    srag_common_node->set_size(pnode->get_size());

    for(set<Node_t>::iterator it = subset.begin(); it != subset.end(); it++){
 	Node_t nbr1 = *it;
	RagNode_t* rag_nbr1= _rag->find_rag_node(nbr1);

    	RagNode_t* srag_node1 = _srag->insert_rag_node(rag_nbr1->get_node_id());
    	_sfeature_mgr->copy_cache(rag_node_cache[rag_nbr1], srag_node1);
    	srag_node1->set_boundary_size(rag_nbr1->get_boundary_size());	
    	srag_node1->set_size(rag_nbr1->get_size());
	
    } 	

    for(set<Node_t>::iterator it = subset.begin(); it != subset.end(); it++){
 	Node_t nbr1 = *it;
	RagNode_t* rag_node1= _rag->find_rag_node(nbr1);
	RagNode_t* srag_node1= _srag->find_rag_node(nbr1);
	int edge_count=0;
	int iter_count=0;

	for(RagNode_t::edge_iterator eit=rag_node1->edge_begin(); eit!=rag_node1->edge_end(); eit++){
	    RagNode_t* other_node = (*eit)->get_other_node(rag_node1);	
	    RagNode_t* srag_other_node = _srag->find_rag_node(other_node->get_node_id());	
	    RagEdge_t* srag_edge1=NULL;	

	    if (other_node->get_node_id() == pnode->get_node_id() ||
		subset.find(other_node->get_node_id()) != subset.end() ){
		if (!_srag->find_rag_edge(srag_node1,srag_other_node)){
    		    srag_edge1 = _srag->insert_rag_edge(srag_node1, srag_other_node);
		    //srag_edge1->print_edge();	
		}
		else if(++edge_count >= (_subsetSz) )
		    break;	  

	    }	
	    if (srag_edge1){
    		srag_edge1->set_weight( (*eit)->get_weight());	
		srag_edge1->set_size((*eit)->get_size());	
    		_sfeature_mgr->copy_cache(rag_edge_cache[(*eit)],srag_edge1);
                if(++edge_count >= (_subsetSz) )
		    break;	  
	    }	
	    iter_count++;
	}
	//printf("degree %d, total iterations %d\n",rag_node1->node_degree(),iter_count);		
    }	
}






int BatchMergeMRFh::oneDaddress(int rr, int cc, int nCols)
{
	int ret;
	ret=(rr * nCols) + cc;
	return ret;
}
void BatchMergeMRFh::ComputeTempIndex(vector< vector<int> > &tupleLabelMat,int nClass,int tupleSz)
{

    int maxRows=(int)pow(nClass*1.0,tupleSz);
    int *tupleLabelMat1D = new int[maxRows*tupleSz];
    int k,i,c,m,consecLen,row,col,oneDidx;
    for (k=1;k<=tupleSz;k++){
	i=1;
	col=k-1;
	while (i <= maxRows)
	    for (c=1;c<=nClass;c++){
		consecLen=(int)pow(nClass*1.0,k-1);
		for (m=0;m<consecLen;m++){
		    row=i-1;
		    
		    oneDidx=oneDaddress(row,tupleSz-1-col,tupleSz);
		    tupleLabelMat1D[oneDidx]=c-1;

		    i++;
		}
	    }
    }
    tupleLabelMat.resize(maxRows);
    for(i=0;i<maxRows;i++){
	tupleLabelMat[i].resize(tupleSz);
	for(k=0;k<tupleSz;k++){
	    int ll=i*tupleSz+k;
	    tupleLabelMat[i][tupleSz-1-k] = tupleLabelMat1D[ll];
	}
    }

    delete[] tupleLabelMat1D;
}


int BatchMergeMRFh::get_gt(RagEdge_t* pedge)
{
    if(_assignment->size()<1)
	return 0;
	
    int ret;
		
    Node_t node1 = pedge->get_node1()->get_node_id();	
    Node_t node2 = pedge->get_node2()->get_node_id();	

    ret = (_assignment->find(node1)->second == _assignment->find(node2)->second)? -1 : 1;

    return ret;  	
}








// //*********************************************************************************************



void BatchMergeMRFh::write_in_file(const char *filename)
{

    FILE *fp=fopen(filename, "wt");

    for (int i=0;i<_subsets.size();i++){
	for(int j=0; j< _subsets[0].size();j++)	
	    fprintf(fp,"%d ",_subsets[i][j]+1);	
	fprintf(fp,"  ");
	for(int j=0; j< _costs[0].size();j++)	
	    fprintf(fp,"%.3f ",_costs[i][j]);	
	fprintf(fp,"\n");
	
    }
    fclose(fp); 	
}

