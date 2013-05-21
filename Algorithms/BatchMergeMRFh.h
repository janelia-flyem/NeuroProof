#ifndef _BATCH_MERGE_MRFh
#define _BATCH_MERGE_MRFh

#include <vector>
#include "../Rag/Rag.h"
#include "../FeatureManager/FeatureManager.h"
#include "../DataStructures/Glb.h"

#define MERGE 0
#define KEEP 1

using namespace std;
using namespace NeuroProof;



class BatchMergeMRFh{

    Rag<Label>* _rag;

    Rag<Label>* _srag;


    FeatureMgr* _feature_mgr;
    FeatureMgr* _sfeature_mgr;

    size_t _subsetSz;	
    double _thd;	
//    int _edgeCount;	

    vector< vector<Label> > _subsets; 	
    vector< vector<double> > _costs; 	
    			
    vector< vector<int> > _labelConfig; 
//    std::vector< std::pair<Label, Label> > _allEdges;	    	

    multimap<Label, Label>* _assignment;

    FILE* _fp; 	
//    vector<double> _edgeWt;
    vector< vector<double> > _edgeBlf;	
    double _update_belief, _bthd;	 	

    multimap<int, vector< vector<int> > > _configList;
public:
    BatchMergeMRFh(Rag<Label>* prag, FeatureMgr* pfmgr, multimap<Label, Label>* assignment, double pthd=0.5, size_t psz=4): _rag(prag), _feature_mgr(pfmgr), _subsetSz(psz), _thd(pthd) {

        _assignment = assignment;
	if (_subsetSz==2){
	    _labelConfig.resize(2*_subsetSz);
	    _labelConfig[0].resize(_subsetSz); _labelConfig[0][0]=MERGE; _labelConfig[0][1] = MERGE;	
	    _labelConfig[1].resize(_subsetSz); _labelConfig[1][0]=KEEP; _labelConfig[1][1] = MERGE;	
	    _labelConfig[2].resize(_subsetSz); _labelConfig[2][0]=MERGE; _labelConfig[2][1] = KEEP;	
	    _labelConfig[3].resize(_subsetSz); _labelConfig[3][0]=KEEP; _labelConfig[3][1] = KEEP;	
	}

	_srag = new Rag<Label>();
	_sfeature_mgr = new FeatureMgr();
	_sfeature_mgr->copy_channel_features(_feature_mgr);
	_sfeature_mgr->set_classifier(_feature_mgr->get_classifier());
	for(int i=2; i<= _subsetSz; i++){
    	    vector< vector<int> > allConfig;
            ComputeTempIndex(allConfig, 2, i);	
	    _configList.insert(make_pair(i, allConfig));	
	}
    };

    double compute_merge_prob( int iterCount, std::vector< std::pair<Label, Label> >& allEdges, string wts_path, string analysis_path);	
    void generate_subsets(RagNode<Label>* pnode);
    void compute_subset_cost(RagNode<Label>* pnode, set<Label>& subset);
    double merge_by_config(vector<int>& config, vector<Label>& subset);
    double merge_by_order(vector<int>& config, vector<Label>& subset, vector<int>& morder);
    void build_srag(RagNode<Label>* pnode, set<Label>& subset);
    int oneDaddress(int rr, int cc, int nCols);
    void ComputeTempIndex(vector< vector<int> > &tupleLabelMat,int nClass,int tupleSz);
    void read_and_set_tree_weights(string sol_fname, vector<double>& tree_wts);
    double refine_edge_weights(std::vector< std::pair<Label, Label> >& allEdges, string tmp_filename);

    void write_in_file(const char *filename);
    int get_gt(RagEdge<Label>* pedge);
};

#endif
