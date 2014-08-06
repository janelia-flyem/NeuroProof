#ifndef FEATUREMGR_H
#define FEATUREMGR_H

#include <boost/python.hpp>

#include "../Rag/RagEdge.h"
#include "Features.h"
#include <tr1/unordered_map>



#include "../Classifier/vigraRFclassifier.h"
#include "../Classifier/opencvRFclassifier.h"
#include "../Classifier/opencvABclassifier.h"
#include "../Classifier/opencvSVMclassifier.h"


// node features and edge features -- different amounts ??!!

namespace boost {

template <typename T>
class shared_ptr;

}

namespace NeuroProof {
    
typedef std::tr1::unordered_map<RagEdge_t*, std::vector<void *>, RagEdgePtrHash<Node_t>, RagEdgePtrEq<Node_t> > EdgeCaches; 
typedef std::tr1::unordered_map<RagNode_t*, std::vector<void *>, RagNodePtrHash<Node_t>, RagNodePtrEq<Node_t> > NodeCaches; 

class FeatureMgr {
  public:
    FeatureMgr() : num_channels(0), specified_features(false),
        has_pyfunc(false), overlap(false), num_features(0),
        overlap_threshold(11), overlap_max(true), eclfr(0), border_weight(1.0) {}
    
    FeatureMgr(int num_channels_) : num_channels(num_channels_), 
        specified_features(false), channels_features(num_channels_),
        channels_features_modes(num_channels_),
        channels_features_equal(num_channels_), has_pyfunc(false),
        overlap(false), num_features(0), overlap_threshold(11),
        overlap_max(true), eclfr(0), border_weight(1.0) {}
    
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

    void set_border_weight(double weight)
    {
        border_weight = weight;
    }
    
    void set_overlap_max()
    {
        overlap_max = true;
    }
    
    void set_overlap_min()
    {
        overlap_max = false;
    }

    unsigned int get_num_channels() const
    {
        return num_channels;
    }

    void set_basic_features();

    string serialize_features(char * current_features, RagNode_t* node)
    {
        string buffer;
        std::vector<void*>& feature_caches = node_caches[node];
        int pos = 0;
        for (int i = 0; i < num_channels; ++i) { 
            std::vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j, ++pos) {
                unsigned int bufsize = features[j]->serialize(current_features,
                        feature_caches[pos], buffer);
                current_features += bufsize; 
            }
        }
        return buffer;
    }

    string serialize_features(char * current_features, RagEdge_t* edge)
    {
        string buffer;
        std::vector<void*>& feature_caches = edge_caches[edge];
        int pos = 0;
        for (int i = 0; i < num_channels; ++i) { 
            std::vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j, ++pos) {
                unsigned int bufsize = features[j]->serialize(current_features,
                        feature_caches[pos], buffer);
                current_features += bufsize; 
            }
        }
        return buffer;
    }


    void deserialize_features(char * current_features, RagNode_t* node)
    {
        string buffer;
        int pos = 0;
        if (node_caches.find(node) == node_caches.end()) {
            std::vector<void*>& feature_caches = create_cache(node);
        }        
        std::vector<void*>& feature_caches = node_caches[node];

        for (int i = 0; i < num_channels; ++i) { 
            std::vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j, ++pos) {
                unsigned int bufsize = features[j]->deserialize(current_features,
                        feature_caches[pos]);
                current_features += bufsize; 
            }
        }
    }


    void deserialize_features(char * current_features, RagEdge_t* edge)
    {
        string buffer;
        int pos = 0;
        if (edge_caches.find(edge) == edge_caches.end()) {
            std::vector<void*>& feature_caches = create_cache(edge);
        }        
        std::vector<void*>& feature_caches = edge_caches[edge];

        for (int i = 0; i < num_channels; ++i) { 
            std::vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j, ++pos) {
                unsigned int bufsize = features[j]->deserialize(current_features,
                        feature_caches[pos]);
                current_features += bufsize; 
            }
        }
    }



    void add_val(double val, RagNode_t* node)
    {
        unsigned int starting_pos = 0;
        if (node_caches.find(node) != node_caches.end()) {
            add_val(val, 0, starting_pos, node_caches[node]);
        } else {
            std::vector<void*>& feature_caches = create_cache(node);
            add_val(val, 0, starting_pos, feature_caches);
        } 
    } 
   
    void add_val(double val, RagEdge_t* edge)
    {
        unsigned int starting_pos = 0;
        if (edge_caches.find(edge) != edge_caches.end()) {
            add_val(val, 0, starting_pos, edge_caches[edge]);
        } else {
            std::vector<void*>& feature_caches = create_cache(edge);
            add_val(val, 0, starting_pos, feature_caches);
        } 
    } 

    void add_val(std::vector<double>& vals, RagNode_t* node)
    {
        //node->incr_size();
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

    void add_val(std::vector<double>& vals, RagEdge_t* edge)
    { 
        //edge->incr_size();
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

    void mv_features(RagEdge_t* edge2, RagEdge_t* edge1);

    void remove_edge(RagEdge_t* edge);

    void remove_node(RagNode_t* node)
    {
        if (node_caches.find(node) != node_caches.end()) {
            std::vector<void*>& node_vec = node_caches[node];
            assert(node_vec.size() > 0);
            unsigned int pos = 0;
            for (int i = 0; i < num_channels; ++i) {
                vector<FeatureCompute*>& features = channels_features[i];
                for (int j = 0; j < features.size(); ++j) {
                    features[j]->delete_cache(node_vec[pos]);
                    ++pos;
                } 
            }
            node_caches.erase(node);
        }
    }

    void merge_features(RagNode_t* node1, RagNode_t* node2);
    void merge_features2(RagNode_t* node1, RagNode_t* node2, RagEdge_t* edge );
    void merge_features(RagEdge_t* edge1, RagEdge_t* edge2);

    void set_classifier(EdgeClassifier* pclfr)
    {
        eclfr = pclfr;
	std::vector<unsigned int> ignore_list;
	eclfr->get_ignore_featlist(ignore_list);
	printf("ignore features: ");
	for(size_t i=0; i<ignore_list.size(); i++){
	    printf("%u  ",ignore_list[i]);
	    ignore_set.insert(ignore_list[i]);
	}
	printf("\n");
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

    double get_prob(RagEdge_t* edge);

    void clear_features();

    ~FeatureMgr();

    void get_responses(RagEdge_t* edge, vector<double>& responses);

    void compute_all_features(RagEdge_t* edge, std::vector<double>&);
    void compute_node_features(RagNode_t* edge, std::vector<double>&);

    void copy_channel_features(FeatureMgr *pfmgr);   	

    void copy_cache(std::vector<void*>& src_edge_cache, RagEdge_t* edge);	
    void copy_cache(std::vector<void*>& src_node_cache, RagNode_t* node);	

    void print_cache(RagEdge_t* edge);	
    void print_cache(RagNode_t* node);	

    std::vector<std::vector<FeatureCompute*> >& get_channel_features(){return channels_features;}; 	
    EdgeCaches& get_edge_cache(){return edge_caches;};  		
    NodeCaches& get_node_cache(){return node_caches;};  		
    
    void find_useless_features(std::vector< std::vector<double> >& all_features);


  private:
    void compute_diff_features(std::vector<void*>* caches1, std::vector<void*>* caches2, std::vector<double>& feature_results, RagEdge_t* edge);
    void compute_diff_features2(std::vector<void*>* caches1, std::vector<void*>* caches2, std::vector<double>& feature_results, RagEdge_t* edge);
     
    void compute_features(unsigned int prediction_type, std::vector<void*>* caches, std::vector<double>& feature_results, RagEdge_t* edge, unsigned int node_num);
    void compute_features2(unsigned int prediction_type, std::vector<void*>* caches, std::vector<double>& feature_results, RagEdge_t* edge, unsigned int node_num);

    void add_val(double val, unsigned int channel, unsigned int& starting_pos, std::vector<void *>& feature_caches)
    {
        std::vector<FeatureCompute*>& features = channels_features[channel];
        for (int i = 0; i < features.size(); ++i) {
            features[i]->add_point(val, feature_caches[starting_pos]); 
            ++starting_pos;
        }
    }
   
  public: 
    // !! assume all edge/node caches
    std::vector<void*>& create_cache(RagEdge_t* edge)
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
    std::vector<void*>& create_cache(RagNode_t* node)
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

  private:
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
    double border_weight;
    std::set<unsigned int> ignore_set;
};

typedef boost::shared_ptr<FeatureMgr> FeatureMgrPtr;

}

#endif


