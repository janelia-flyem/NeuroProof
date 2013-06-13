#ifndef RAGUTILS2_H
#define RAGUTILS2_H


#include "../FeatureManager/FeatureManager.h"
#include "../Rag/RagUtils.h"
#include "../Algorithms/MergePriorityFunction.h"
#include "../Algorithms/MergePriorityQueue.h"
#include "../BioPriors/MitoTypeProperty.h"

namespace NeuroProof {

typedef std::multimap<double, std::pair<unsigned int, unsigned int> > EdgeRank_t; 

struct EdgeRanking {
    typedef std::multimap<double, RagEdge_uit* > type;
};


void rag_add_edge(Rag_uit* rag, unsigned int id1, unsigned int id2, std::vector<double>& preds, 
        FeatureMgr * feature_mgr); 

double mito_boundary_ratio(RagEdge_uit* edge);

// assume that 0 body will never be added as a screen
EdgeRanking::type rag_grab_edge_ranking(Rag_uit& rag, double min_val, double max_val, double start_val, double ignore_size=27, Node_uit screen_id = 0);

}


#endif
