#ifndef STACKLEARNALGS_H
#define STACKLEARNALGS_H

#include "../Utilities/unique_row_matrix.h"
#include <vector>

namespace NeuroProof {

class StackController;

void learn_edge_classifier_flat(BioStackController& controller, double threshold,
        UniqueRowFeature_Label& all_featuresu, std::vector<int>& all_labels, bool use_mito);

void learn_edge_classifier_queue(BioStackController& controller, double threshold,
        UniqueRowFeature_Label& all_featuresu, std::vector<int>& all_labels,
        bool accumulate_all, bool use_mito);

void learn_edge_classifier_lash(BioStackController& controller, double threshold,
        UniqueRowFeature_Label& all_featuresu, std::vector<int>& all_labels, bool use_mito);

}

#endif
