#ifndef FEATUREMANAGER_H
#define FEATUREMANAGER_H

#include <boost/python.hpp>

#include "../Rag/RagEdge.h"
#include "../DataStructures/Glb.h"
#include "Features.h"
#include <tr1/unordered_map>



#include "../Classifier/vigraRFclassifier.h"
#include "../Classifier/opencvRFclassifier.h"
#include "../Classifier/opencvABclassifier.h"
#include "../Classifier/opencvSVMclassifier.h"


// node features and edge features -- different amounts ??!!


namespace NeuroProof {
    
typedef std::tr1::unordered_map<RagEdge_uit*, std::vector<void *>, RagEdgePtrHash<Label>, RagEdgePtrEq<Label> > EdgeCaches; 
typedef std::tr1::unordered_map<RagNode_uit*, std::vector<void *>, RagNodePtrHash<Label>, RagNodePtrEq<Label> > NodeCaches; 

class FeatureMgr {
  public:
    FeatureMgr() : num_channels(0), specified_features(false), has_pyfunc(false), overlap(false), num_features(0), overlap_threshold(11), overlap_max(true), eclfr(0) {}
    FeatureMgr(int num_channels_) : num_channels(num_channels_), specified_features(false), channels_features(num_channels_), channels_features_modes(num_channels_), has_pyfunc(false), overlap(false), num_features(0), overlap_threshold(11), overlap_max(true), eclfr(0) {}
    
    void add_channel();
    unsigned int get_num_features()
    {
        return num_features;
    }
#ifdef SETPYTHON
    void set_python_rf_function(boost::python::object pyfunc_);
#endif
    void set_overlap_function();
    void set_overlap_cutoff(int threshold)
    {
        overlap_threshold = threshold;
    }
    
    void set_overlap_max()
    {
        overlap_max = true;
    }
    
    void set_overlap_min()
    {
        overlap_max = false;
    }

    unsigned int get_num_channels()
    {
        return num_channels;
    }

    void add_val(double val, RagNode_uit* node)
    {
        unsigned int starting_pos = 0;
        if (node_caches.find(node) != node_caches.end()) {
            add_val(val, 0, starting_pos, node_caches[node]);
        } else {
            std::vector<void*>& feature_caches = create_cache(node);
            add_val(val, 0, starting_pos, feature_caches);
        } 
    } 
    
    void add_val(double val, RagEdge_uit* edge)
    {
        unsigned int starting_pos = 0;
        if (edge_caches.find(edge) != edge_caches.end()) {
            add_val(val, 0, starting_pos, edge_caches[edge]);
        } else {
            std::vector<void*>& feature_caches = create_cache(edge);
            add_val(val, 0, starting_pos, feature_caches);
        } 
    } 

    void add_val(std::vector<double>& vals, RagNode_uit* node)
    {
        node->incr_size();
        assert(vals.size() == num_channels);
        unsigned starting_pos = 0;
        if (node_caches.find(node) != node_caches.end()) {
            std::vector<void*>& feature_caches = node_caches[node];
            for (int i = 0; i < num_channels; ++i) { 
                add_val(vals[i], i, starting_pos, feature_caches);
            }              
        } else {
            std::vector<void*>& feature_caches = create_cache(node);
            for (int i = 0; i < num_channels; ++i) { 
                add_val(vals[i], i, starting_pos, feature_caches);
            }
        }        
    }

    void add_val(std::vector<double>& vals, RagEdge_uit* edge)
    { 
        edge->incr_size();
        assert(vals.size() == num_channels);
        unsigned int starting_pos = 0;
        if (edge_caches.find(edge) != edge_caches.end()) {
            std::vector<void*>& feature_caches = edge_caches[edge];
            for (int i = 0; i < num_channels; ++i) { 
                add_val(vals[i], i, starting_pos, feature_caches);
            }           
        } else {
            std::vector<void*>& feature_caches = create_cache(edge);
            for (int i = 0; i < num_channels; ++i) { 
                add_val(vals[i], i, starting_pos, feature_caches);
            }        
        } 
    }

    void mv_features(RagEdge_uit* edge2, RagEdge_uit* edge1)
    {
        edge1->set_size(edge2->get_size());
        if (edge_caches.find(edge2) != edge_caches.end()) {
            edge_caches[edge1] = edge_caches[edge2];
            edge_caches.erase(edge2);        
        }
    } 

    void remove_edge(RagEdge_uit* edge)
    {
        if (edge_caches.find(edge) != edge_caches.end()) {
            std::vector<void*>& edge_vec = edge_caches[edge];
            assert(edge_vec.size() > 0);
            unsigned int pos = 0;
            for (int i = 0; i < num_channels; ++i) {
                vector<FeatureCompute*>& features = channels_features[i];
                for (int j = 0; j < features.size(); ++j) {
                    features[j]->delete_cache(edge_vec[pos]);
                    ++pos;
                } 
            }
            edge_caches.erase(edge);
        }
    }

    void remove_node(RagNode_uit* node)
    {
        node_caches.erase(node);
    }

    void merge_features(RagNode_uit* node1, RagNode_uit* node2);
    void merge_features2(RagNode_uit* node1, RagNode_uit* node2, RagEdge_uit* edge );
    void merge_features(RagEdge_uit* edge1, RagEdge_uit* edge2);

    void set_classifier(EdgeClassifier* pclfr)
    {
        eclfr = pclfr;
    }

    EdgeClassifier* get_classifier()
    {
        return eclfr;
    }  	

    //void set_tree_weights(std::vector<double>& pwts){
    //	eclfr->set_tree_weights(pwts);
    //}



    void add_median_feature();

#ifdef SETPYTHON
    void add_hist_feature(unsigned int num_bins, boost::python::list percentiles, bool use_diff);
#else
    void add_hist_feature(unsigned int num_bins, std::vector<double> percentiles, bool use_diff);
#endif

    void add_moment_feature(unsigned int num_moments, bool use_diff);
    
    void add_inclusiveness_feature(bool use_diff);

    double get_prob(RagEdge_uit* edge);

    void clear_features();

    ~FeatureMgr();

    void get_responses(RagEdge_uit* edge, vector<double>& responses);

    void compute_all_features(RagEdge_uit* edge, std::vector<double>&);
    void compute_node_features(RagNode_uit* edge, std::vector<double>&);

    void copy_channel_features(FeatureMgr *pfmgr);   	

    void copy_cache(std::vector<void*>& src_edge_cache, RagEdge_uit* edge);	
    void copy_cache(std::vector<void*>& src_node_cache, RagNode_uit* node);	

    void print_cache(RagEdge_uit* edge);	
    void print_cache(RagNode_uit* node);	

    std::vector<std::vector<FeatureCompute*> >& get_channel_features(){return channels_features;}; 	
    EdgeCaches& get_edge_cache(){return edge_caches;};  		
    NodeCaches& get_node_cache(){return node_caches;};  		


  private:
    void compute_diff_features(std::vector<void*>* caches1, std::vector<void*>* caches2, std::vector<double>& feature_results, RagEdge_uit* edge);
    void compute_diff_features2(std::vector<void*>* caches1, std::vector<void*>* caches2, std::vector<double>& feature_results, RagEdge_uit* edge);
     
    void compute_features(unsigned int prediction_type, std::vector<void*>* caches, std::vector<double>& feature_results, RagEdge_uit* edge, unsigned int node_num);
    void compute_features2(unsigned int prediction_type, std::vector<void*>* caches, std::vector<double>& feature_results, RagEdge_uit* edge, unsigned int node_num);

    void add_val(double val, unsigned int channel, unsigned int& starting_pos, std::vector<void *>& feature_caches)
    {
        std::vector<FeatureCompute*>& features = channels_features[channel];
        for (int i = 0; i < features.size(); ++i) {
            features[i]->add_point(val, feature_caches[starting_pos]); 
            ++starting_pos;
        }
    }
    
    // !! assume all edge/node caches
    std::vector<void*>& create_cache(RagEdge_uit* edge)
    {
        edge_caches[edge] = std::vector<void*>();
        std::vector<void*>& caches = edge_caches[edge];
        unsigned int pos = 0;
        for (unsigned int i = 0; i < num_channels; ++i) {
            std::vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j) {
                caches.push_back(features[j]->create_cache());
                ++pos;
            } 
        }
        return caches;
    }
    std::vector<void*>& create_cache(RagNode_uit* node)
    {
        node_caches[node] = std::vector<void*>();
        std::vector<void*>& caches = node_caches[node];
        unsigned int pos = 0;
        for (unsigned int i = 0; i < num_channels; ++i) {
            std::vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j) {
                caches.push_back(features[j]->create_cache());
                ++pos;
            } 
        }
        return caches;
    }    

    void add_feature(unsigned int channel, FeatureCompute * feature, std::vector<bool>& feature_modes);


    EdgeCaches edge_caches;
    NodeCaches node_caches;
    unsigned int num_features;

    bool specified_features;
    unsigned int num_channels;

    std::vector<std::vector<FeatureCompute*> > channels_features;
    std::vector<std::vector<FeatureCompute*> > channels_features_equal;
    std::vector<std::vector<std::vector<bool> > > channels_features_modes;

#ifdef SETPYTHON
    boost::python::object pyfunc;
#endif
    bool has_pyfunc;
    bool overlap;
    bool overlap_max;
    int overlap_threshold;
    EdgeClassifier* eclfr;	 
};








}

#endif


