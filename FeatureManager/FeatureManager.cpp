#include "FeatureManager.h"

using std::vector;
using namespace NeuroProof;

void FeatureMgr::add_channel()
{
    if (!specified_features) {
        throw ErrMsg("Cannot add a channel after adding features");
    }

    vector<FeatureCompute*> features_temp;
    vector<vector<bool> > features_modes_temp;   
 
    channels_features.push_back(features_temp);
    channels_features_modes.push_back(features_modes_temp);

    ++num_channels;
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
void FeatureMgr::add_hist_feature(unsigned int num_bins, std::vector<double>& percentiles, bool use_diff)
{
    vector<bool> feature_modes(3, true);
    feature_modes[2] = use_diff;
    
    for (unsigned int i = 0; i < num_channels; ++i) {
        add_feature(num_channel, new FeatureHist(num_bins, percentiles), feature_modes);
    }
}

void FeatureMgr::add_moment_feature(unsigned int num_moments, bool use_diff)
{
    vector<bool> feature_modes(3, true);
    feature_modes[2] = use_diff;

    for (unsigned int i = 0; i < num_channels; ++i) {
        add_feature(num_channel, new FeatureMoment(num_moments), feature_modes);
    }
}



void FeatureMgr::add_feature(unsigned int channel, FeatureCompute * feature, vector<bool>& feature_modes)
{
    specified_features = true; 
    assert(channel < num_channels);

    channels_features[channel].push_back(feature); 
    channels_features[channel].push_back(feature_modes);
}

void FeatureMgr::set_python_rf_function(object pyfunc_)
{
    pyfunc = pyfunc_;
    has_pyfunc = true;
}


double FeatureMgr::get_prob(RagEdge<Label>* edge)
{
    std::vector<void*>* edget_caches = 0; 
    std::vector<void*>* node1_caches = 0; 
    std::vector<void*>* node2_caches = 0;
 
    if (edge_caches.find(edge) != edge_cached.end()) {
        edget_caches = &(edge_caches[edge]);
    }

    RagNode<Label>* node1 = edge->get_node1();
    RagNode<Label>* node2 = edge->get_node2();

    if (node_caches.find(node1) != node_cached.end()) {
        node1_caches = &(node_caches[node1]);
    }
    if (node_caches.find(node2) != node_cached.end()) {
        node2_caches = &(node_caches[node2]);
    }

    unsigned int pos = 0;
    vector<double> feature_results;
    for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[channel];
        vector<vector<bool> >& features_modes = channels_features_modes[channel];

        for (int j = 0; j < features.size(); ++j) {
            if (features_modes[0]) {
                features[j]->get_feature_array((*node1_caches)[pos], feature_results);
                features[j]->get_feature_array((*node2_caches)[pos], feature_results);
            }
            if (features_modes[1]) {
                features[j]->get_feature_array((*edget_caches)[pos], feature_results);
            }
            if (features_modes[2]) {
                features[j]->get_diff_feature_array((*node1_caches)[pos], (*node2_caches)[pos], feature_results);
            }           
            ++pos;
        }
    } 

    double prob = 0.0;
    if (has_pyfunc) {
        boost::python::list pylist;
        for (int i = 0; feature_results.size(); ++i) {
            pylist.append(feature_results[i]);
        }
        prob = pyfunc(pylist);
    } else {
        prob = feature_results[0];
    }

    return prob;
}

void FeatureMgr::merge_features(RagNode<Label>* node1, RagNode<Label>* node2)
{
    std::vector<void*>* node1_caches = 0; 
    std::vector<void*>* node2_caches = 0;
 
    if (node_caches.find(node1) != node_cached.end()) {
        node1_caches = &(node_caches[node1]);
    }
    if (node_caches.find(node2) != node_cached.end()) {
        node2_caches = &(node_caches[node2]);
    }
    if (!node1_caches && !node2_caches) {
        return;
    }

    unsigned int pos = 0;
    vector<double> feature_results;
    for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[channel];
        for (int j = 0; j < features.size(); ++j) {
            if (node1_caches[pos] && node2_caches[pos]) {
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
 
    if (edge_caches.find(edge1) != edge_cached.end()) {
        edge1_caches = &(edge_caches[edge1]);
    }
    if (edge_caches.find(edge2) != edge_cached.end()) {
        edge2_caches = &(edge_caches[edge2]);
    }

    unsigned int pos = 0;
    vector<double> feature_results;
    for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[channel];
        for (int j = 0; j < features.size(); ++j) {
            if (edge1_caches[pos] && edge2_caches[pos]) {
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
        vector<FeatureCompute*>& features = channels_features[channel];
            for (int j = 0; j < features.size(); ++j) {
                features[j]->delete_cache(iter->second[pos]);
                ++pos;
            } 
        }
    }

    for (NodeCaches::iterator iter = node_caches.begin(); iter != node_caches.end(); ++iter) {
        unsigned int pos = 0;
        for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[channel];
            for (int j = 0; j < features.size(); ++j) {
                features[j]->delete_cache(iter->second[pos]);
                ++pos;
            } 
        }
    }

}


