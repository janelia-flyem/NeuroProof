#include "FeatureManager.h"

using std::vector;
using namespace NeuroProof;
#ifdef SETPYTHON
using namespace boost::python;
#endif 

// ?! assume every feature is on every channel -- for now

#include <iostream>
void FeatureMgr::add_channel()
{
    if (specified_features) {
        throw ErrMsg("Cannot add a channel after adding features");
    }

    vector<FeatureCompute*> features_temp;
    vector<vector<bool> > features_modes_temp;   
 
    channels_features.push_back(features_temp);
    channels_features_equal.push_back(features_temp);
    channels_features_modes.push_back(features_modes_temp);

    ++num_channels;
}

void FeatureMgr::compute_diff_features(std::vector<void*>* caches1, std::vector<void*>* caches2, std::vector<double>& feature_results)
{
    vector<vector<bool> > examine_equal(num_channels);
    vector<vector<unsigned int> > spot_equal(num_channels);

    // !! stupid hack to put features in the 'correct' order
    unsigned int pos = 0;
    for (int j = 0; j < num_channels; ++j) {
        unsigned int count = 0;
        for (int i = 0; i < channels_features_equal[0].size(); ++i) {
            spot_equal[j].push_back(pos);
            if (channels_features_equal[j][i]) {
                ++pos;
                if (channels_features_modes[j][count][2]) {
                    examine_equal[j].push_back(true);
                } else {
                    examine_equal[j].push_back(false);
                } 
                ++count;
            } else {
                examine_equal[j].push_back(false);
            }
        }
    }

    for (int i = 0; i < channels_features_equal[0].size(); ++i) {
        for (int j = 0; j < num_channels; ++j) {
            if (examine_equal[j][i]) {
                unsigned int id = spot_equal[j][i];
                channels_features_equal[j][i]->get_diff_feature_array((*caches1)[id], (*caches2)[id], feature_results);
            }
        }
    }
} 

void FeatureMgr::compute_features(unsigned int prediction_type, std::vector<void*>* caches, std::vector<double>& feature_results)
{
    vector<vector<bool> > examine_equal(num_channels);
    vector<vector<unsigned int> > spot_equal(num_channels);

    // !! stupid hack to put features in the 'correct' order
    unsigned int pos = 0;
    for (int j = 0; j < num_channels; ++j) {
        unsigned int count = 0;
        for (int i = 0; i < channels_features_equal[0].size(); ++i) {
            spot_equal[j].push_back(pos);
            if (channels_features_equal[j][i]) {
                ++pos;
                if (channels_features_modes[j][count][prediction_type]) {
                    examine_equal[j].push_back(true);
                } else {
                    examine_equal[j].push_back(false);
                } 
                ++count;
            } else {
                examine_equal[j].push_back(false);
            }
        }
    }

    for (int i = 0; i < channels_features_equal[0].size(); ++i) {
        for (int j = 0; j < num_channels; ++j) {
            if (examine_equal[j][i]) {
                unsigned int id = spot_equal[j][i];
                channels_features_equal[j][i]->get_feature_array((*caches)[id], feature_results);
            }
        }
    }
} 


void FeatureMgr::add_median_feature()
{
    if (num_channels != 1) {
        throw ErrMsg("Median filter can only be added if there is one channel");
    }

    vector<bool> feature_modes(3, false);
    feature_modes[1] = true;
    std::vector<double> percentiles;
    percentiles.push_back(0.5);
    add_feature(0, new FeatureHist(100, percentiles), feature_modes);
}

#ifdef SETPYTHON
void FeatureMgr::add_hist_feature(unsigned int num_bins, boost::python::list percentiles, bool use_diff)
{
    vector<bool> feature_modes(3, true);
    feature_modes[2] = use_diff;
    
    unsigned int num_percentiles = len(percentiles);

    vector<double> percentiles_vec;

    for (unsigned int i = 0; i < num_percentiles; ++i) {
        percentiles_vec.push_back(extract<double>(percentiles[i]));
    }
    FeatureCompute * feature_ptr = new FeatureHist(num_bins, percentiles_vec);
    for (unsigned int i = 0; i < num_channels; ++i) {
        channels_features_equal[i].push_back(feature_ptr);
        add_feature(i, feature_ptr, feature_modes);
    }
}
#endif

void FeatureMgr::add_moment_feature(unsigned int num_moments, bool use_diff)
{
    vector<bool> feature_modes(3, true);
    feature_modes[2] = use_diff;

    FeatureCompute * feature_ptr0 = new FeatureCount;
    add_feature(0, feature_ptr0, feature_modes);
    channels_features_equal[0].push_back(feature_ptr0);
    for (unsigned int i = 1; i < num_channels; ++i) {
        channels_features_equal[i].push_back(0);
    }
    FeatureCompute * feature_ptr = new FeatureMoment(num_moments);
    for (unsigned int i = 0; i < num_channels; ++i) {
        channels_features_equal[i].push_back(feature_ptr);
        add_feature(i, feature_ptr, feature_modes);
    }
}



void FeatureMgr::add_feature(unsigned int channel, FeatureCompute * feature, vector<bool>& feature_modes)
{
    specified_features = true; 
    assert(channel < num_channels);
    ++num_features;

    channels_features[channel].push_back(feature); 
    channels_features_modes[channel].push_back(feature_modes);
}

#ifdef SETPYTHON

void FeatureMgr::set_python_rf_function(object pyfunc_)
{
    pyfunc = pyfunc_;
    has_pyfunc = true;
}

#endif

double FeatureMgr::get_prob(RagEdge<Label>* edge)
{
    std::vector<void*>* edget_caches = 0;
    std::vector<void*>* node1_caches = 0;
    std::vector<void*>* node2_caches = 0;

    if (edge_caches.find(edge) != edge_caches.end()) {
        edget_caches = &(edge_caches[edge]);
    }

    RagNode<Label>* node1 = edge->get_node1();
    RagNode<Label>* node2 = edge->get_node2();

    if (node2->get_size() < node1->get_size()) {
        RagNode<Label>* temp_node = node2;
        node2 = node1;
        node1 = temp_node;
    }

    if (node_caches.find(node1) != node_caches.end()) {
        node1_caches = &(node_caches[node1]);
    }
    if (node_caches.find(node2) != node_caches.end()) {
        node2_caches = &(node_caches[node2]);
    }

    vector<double> feature_results;
    
    compute_features(0, node1_caches, feature_results);
    compute_features(0, node2_caches, feature_results);
    compute_features(1, edget_caches, feature_results);
    compute_diff_features(node1_caches, node2_caches, feature_results);

    double prob = 0.0;
    if (has_pyfunc) {
#ifdef SETPYTHON
        /*
        object get_iter = iterator<vector<double> >();
        object iter = get_iter(feature_results);
        list pylist(iter);
*/
        boost::python::list pylist;
        for (unsigned int i = 0; i < feature_results.size(); ++i) {
            pylist.append(feature_results[i]);
        }
        prob = extract<double>(pyfunc(pylist));
#endif
    } else {
        prob = feature_results[0];
    }

    return prob;
}

void FeatureMgr::merge_features(RagNode<Label>* node1, RagNode<Label>* node2)
{
    std::vector<void*>* node1_caches = 0; 
    std::vector<void*>* node2_caches = 0;
 
    if (node_caches.find(node1) != node_caches.end()) {
        node1_caches = &(node_caches[node1]);
    }
    if (node_caches.find(node2) != node_caches.end()) {
        node2_caches = &(node_caches[node2]);
    }
    if (!node1_caches && !node2_caches) {
        return;
    }

    unsigned int pos = 0;
    vector<double> feature_results;
    for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
            if ((*node1_caches)[pos] && (*node2_caches)[pos]) {
                features[j]->merge_cache((*node1_caches)[pos], (*node2_caches)[pos]);
            }
            ++pos;
        }
    }

    if (node2_caches) {
        node_caches.erase(node2);
    }
}

void FeatureMgr::merge_features(RagEdge<Label>* edge1, RagEdge<Label>* edge2)
{
    std::vector<void*>* edge1_caches = 0; 
    std::vector<void*>* edge2_caches = 0;
 
    if (edge_caches.find(edge1) != edge_caches.end()) {
        edge1_caches = &(edge_caches[edge1]);
    }
    if (edge_caches.find(edge2) != edge_caches.end()) {
        edge2_caches = &(edge_caches[edge2]);
    }

    unsigned int pos = 0;
    vector<double> feature_results;
    for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
            if ((*edge1_caches)[pos] && (*edge2_caches)[pos]) {
                features[j]->merge_cache((*edge1_caches)[pos], (*edge2_caches)[pos]);
            }
            ++pos;
        }
    }

    if (edge2_caches) {
        edge_caches.erase(edge2);
    }
}

FeatureMgr::~FeatureMgr()
{
    for (EdgeCaches::iterator iter = edge_caches.begin(); iter != edge_caches.end(); ++iter) {
        unsigned int pos = 0;
        for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j) {
                features[j]->delete_cache(iter->second[pos]);
                ++pos;
            } 
        }
    }

    for (NodeCaches::iterator iter = node_caches.begin(); iter != node_caches.end(); ++iter) {
        unsigned int pos = 0;
        for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j) {
                features[j]->delete_cache(iter->second[pos]);
                ++pos;
            } 
        }
    }

}


