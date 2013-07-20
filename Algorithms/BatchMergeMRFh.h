/*!
 * Agglomeration ordering algorithm.
 *
 * \author Toufiq Parag (paragt@janelia.hhmi.org)
*/

#ifndef _BATCH_MERGE_MRFh
#define _BATCH_MERGE_MRFh

#include "../FeatureManager/FeatureMgr.h"
#include <vector>
#include "../Rag/Rag.h"

#define MERGE 0
#define KEEP 1

using namespace std;
using namespace NeuroProof;

class BatchMergeMRFh{
  public:
    BatchMergeMRFh(Rag_t* prag, FeatureMgr* pfmgr,
            multimap<Node_t, Node_t>* assignment,
            double pthd=0.5, size_t psz=4);

    double compute_merge_prob( int iterCount, std::vector< std::pair<Node_t, Node_t> >& allEdges, string wts_path, string analysis_path);	
    void generate_subsets(RagNode_t* pnode);
    void compute_subset_cost(RagNode_t* pnode, set<Node_t>& subset);
    double merge_by_config(vector<int>& config, vector<Node_t>& subset);
    double merge_by_order(vector<int>& config, vector<Node_t>& subset, vector<int>& morder);
    void build_srag(RagNode_t* pnode, set<Node_t>& subset);
    int oneDaddress(int rr, int cc, int nCols);
    void ComputeTempIndex(vector< vector<int> > &tupleLabelMat,int nClass,int tupleSz);
    void read_and_set_tree_weights(string sol_fname, vector<double>& tree_wts);
    double refine_edge_weights(std::vector< std::pair<Node_t, Node_t> >& allEdges, string tmp_filename);

    void write_in_file(const char *filename);
    int get_gt(RagEdge_t* pedge);
  
  private:
    Rag_t* _rag;

    Rag_t* _srag;


    FeatureMgr* _feature_mgr;
    FeatureMgr* _sfeature_mgr;

    size_t _subsetSz;	
    double _thd;	
//    int _edgeCount;	

    vector< vector<Node_t> > _subsets; 	
    vector< vector<double> > _costs; 	
    			
    vector< vector<int> > _labelConfig; 

    multimap<Node_t, Node_t>* _assignment;

    FILE* _fp; 	
//    vector<double> _edgeWt;
    vector< vector<double> > _edgeBlf;	
    double _update_belief, _bthd;	 	

    multimap<int, vector< vector<int> > > _configList;
};

#endif
