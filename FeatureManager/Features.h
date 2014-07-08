#ifndef FEATURES_H
#define FEATURES_H

#include "FeatureCache.h"
#include "../Utilities/ErrMsg.h"
#include "../Rag/RagEdge.h"
#include <vector>
#include <set>

namespace NeuroProof {

class FeatureCompute {
  public:
    virtual void * create_cache() = 0;
    virtual void copy_cache(void* src, void* dest)=0;  	
    virtual void delete_cache(void * cache) = 0;
    virtual void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0) = 0;
    virtual void  get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge_t* edge, unsigned int node_num) = 0; 
    virtual void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge_t* edge) = 0; 
    // will delete second cache
    virtual void merge_cache(void * cache1, void * cache2) = 0; 
    virtual void print_cache(void* pcache) = 0; 	
    virtual void print_name() = 0; 	
    // serialize feature and combine with bytes if not 0
    size_t serialize(char * bytes, void* cache1, std::string& buffer);
    size_t deserialize(char * bytes, void * cache1);
    virtual ~FeatureCompute() {}
};


class FeatureHist : public FeatureCompute {
  public:
    FeatureHist(int num_bins_, const std::vector<double>& thresholds_) : num_bins(num_bins_), thresholds(thresholds_) {} 
  
    void * create_cache();
    void copy_cache(void* src, void* dest);  	
    void delete_cache(void * cache);
    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0);
    void get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge_t* edge, unsigned int node_num);
    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge_t* edge);
    void merge_cache(void * cache1, void * cache2);
    void print_name();	
    void print_cache(void* pcache);

  private:
    double get_data(HistCache * hist_cache, double threshold);
      
    int num_bins;
    std::vector<double> thresholds; 
};

// !! temporary support only 0, 1, 2, 3, and 4
class FeatureMoment : public FeatureCompute {
  public:
    FeatureMoment(int num_moments_) : num_moments(num_moments_)
    {
        assert((num_moments <= 4) && (num_moments > 0));
    } 
    
    void * create_cache();
    void copy_cache(void* src, void* dest);  	
    void delete_cache(void * cache);
    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0);
    void get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge_t* edge, unsigned int node_num);
    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge_t* edge);
    void merge_cache(void * cache1, void * cache2);
    void print_name();
    void print_cache(void* pcache);

  private:
    void get_data(MomentCache * moment_cache, std::vector<double>& feature_array);
          
    unsigned int num_moments;
};


class FeatureInclusiveness : public FeatureCompute {
  public:
    FeatureInclusiveness() {} 
    void * create_cache()
    {
        return 0;
    }
    void delete_cache(void * cache)
    {
        return;
    }
    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
    {
        return;
    }
    void copy_cache(void* src, void* dest) {};  	
   
    void get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge_t* edge, unsigned int node_num);
    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge_t* edge);

    void merge_cache(void * cache1, void * cache2)
    {
        return;
    }
    
    void print_name();
    void print_cache(void* pcache) {}

  private:
    void get_node_features(RagNode_t* node, std::vector<double>& features);
    unsigned long long get_lengths(RagNode_t* node, std::set<unsigned long long>& lengths);
   
};


class FeatureCount : public FeatureCompute {
  public:
    FeatureCount() {} 
    void * create_cache()
    {
        return (void*)(new CountCache());
    }
    void copy_cache(void* src, void* dest)
    {
        ((CountCache*)dest)->count = ((CountCache*)src)->count;
    } 	
    void delete_cache(void * cache)
    {
        delete (CountCache*)(cache);
    }

    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0);
    
    void get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge_t* edge, unsigned int node_num);

    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge_t* edge);

    void merge_cache(void * cache1, void * cache2);
    
    void print_name();
    void print_cache(void *pcache) {}	
};




}




#endif
