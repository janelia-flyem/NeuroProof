#ifndef FEATUREMANAGER_H
#define FEATUREMANAGER_H


#include "../DataStructures/RagEdge.h"

// node features and edge features -- different amounts ??!!


namespace NeuroProof {

class FeatureMgr {
  public:
    FeatureMgr() : num_channels(0), specified_features(false), has_pyfunc(false) {}
    FeatureMgr(int num_channels_) : num_channels(num_channels_), specified_features(false), channels_features(num_channels_), channels_features_modes(num_channels_), has_pyfunc(false) {}
    
    void add_channel(std::string channel_name);
    void set_python_rf_function(object pyfunc_);

    void get_num_channels()
    {
        return num_channels;
    }

    void add_val(double val, RagNode<Label>* node)
    {
        unsigned int starting_pos = 0;
        if (node_caches.find(node) != node_cached.end()) {
            add_val(val, 0, starting_pos, node_caches[node]);
        } else {
            std::vector<void*>& feature_caches = create_cache(node);
            add_val(val, 0, starting_pos, feature_caches);
        } 
    } 
    
    void add_val(double val, RagEdge<Label>* edge)
    {
        unsigned int starting_pos = 0;
        if (edge_caches.find(edge) != edge_caches.end()) {
            add_val(val, 0, staring_pos, edge_caches[edge]);
        } else {
            std::vector<void*>& feature_caches = create_cache(edge);
            add_val(val, 0, starting_pos, feature_caches);
        } 
    } 

    void add_val(std::vector<double>& vals, RagNode<Label>* node)
    {
        assert(vals.size() == num_channels);
        unsigned starting_pos = 0;
        if (node_caches.find(node) != node_cached.end()) {
            std::vector<void*>& feature_caches = node_caches[node];
            for (int i = 0; i < num_channels; ++i) { 
                add_val(val, i, starting_pos, feature_caches);
            }              
        } else {
            std::vector<void*>& feature_caches = create_cache(node);
            for (int i = 0; i < num_channels; ++i) { 
                add_val(val, i, starting_pos, feature_caches);
            }
        }        
    }

    void add_val(std::vector<double>& vals, RagEdge<Label>* edge)
    { 
        assert(vals.size() == num_channels);
        unsigned int starting_pos = 0;
        if (edge_caches.find(edge) != edge_caches.end()) {
            std::vector<void*>& feature_caches = edge_caches[edge];
            for (int i = 0; i < num_channels; ++i) { 
                add_val(val, i, starting_pos, feature_caches);
            }           
        } else {
            std::vector<void*>& feature_caches = create_cache(edge);
            for (int i = 0; i < num_channels; ++i) { 
                add_val(val, i, starting_pos, feature_caches);
            }        
        } 
    }

    void mv_features(RagEdge<Label>* edge2, RagEdge<Label>* edge1)
    {
        edge_caches[edge1] = edge_caches[edge2]
        edge_caches.erase(edge2);        
    } 

    void merge_features(RagEdge<Label>* edge1, RagNode<Label>* edge2);
    void merge_features(RagNode<Label>* node1, RagNode<Label>* node2);

    void add_median_feature();
    void add_hist_feature(unsigned int num_bins, std::vector<double>& percentiles, bool use_diff);
    void add_moment_feature(unsigned int num_moments, bool use_diff);

    double get_prob(RagEdge<Label>* edge);

    ~FeatureManager();

  private:

    void add_val(double val, unsigned int channel, unsigned int& starting_pos, std::vector<void *>& feature_caches)
    {
        std::vector<FeatureCompute*>& features = channels_features[channel];
        for (int i = 0; i < features.size(); ++i) {
            features[i]->add_point(val, feature_caches[starting_pos]); 
            ++starting_pos;
        }
    }
    
    // !! assume all edge/node caches
    std::vector<void*>& create_cache(RagEdge* edge)
    {
        edge_caches[edge] = std::vector<void*>();
        std::vector<void*>& caches = edge_caches[edge];
        unsigned int pos = 0;
        for (unsigned int i = 0; i < num_channels; ++i) {
            vector<FeatureCompute*>& features = channels_features[channel];
            for (int j = 0; j < features.size(); ++j) {
                caches.push_back(features[j]->create_cache());
                ++pos;
            } 
        }
        return caches;
    }
    std::vector<void*>& create_cache(RagNode* node)
    {
        node_caches[node] = std::vector<void*>();
        std::vector<void*>& caches = node_caches[node];
        unsigned int pos = 0;
        for (unsigned int i = 0; i < num_channels; ++i) {
            vector<FeatureCompute*>& features = channels_features[channel];
            for (int j = 0; j < features.size(); ++j) {
                caches.push_back(features[j]->create_cache());
                ++pos;
            } 
        }
        return caches;
    }    

    void add_feature(unsigned int channel, FeatureCompute * feature);

    typedef std::tr1::unordered_map<RagEdge<Label>*, std::vector<void *> feature_caches, RagEdgePtrHash<Label>, RagEdgePtrEq<Label> > EdgeCaches; 
    typedef std::tr1::unordered_map<RagNode<Label>*, std::vector<void *> feature_caches, RagNodePtrHash<Label>, RagNodePtrEq<Label> > NodeCaches; 

    EdgeCaches edge_caches;
    NodeCaches node_caches;

    bool specified_features;
    unsigned int num_channels;

    std::vector<std::vector<FeatureCompute*> > channels_features;
    std::vector<std::vector<std::vector<bool> > > channels_features_modes;

    object pyfunc;
    bool has_pyfunc;
};








}

#endif


