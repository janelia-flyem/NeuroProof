#ifndef FEATURES_H
#define FEATURES_H

#include "FeatureCache.h"
#include "../Utilities/ErrMsg.h"
#include <vector>
#include <set>

namespace NeuroProof {

class FeatureCompute {
  public:
    virtual void * create_cache() = 0;
    virtual void delete_cache(void * cache) = 0;
    virtual void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0) = 0;
    virtual void  get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge<Label>* edge, unsigned int node_num) = 0; 
    virtual void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge<Label>* edge) = 0; 
    // will delete second cache
    virtual void merge_cache(void * cache1, void * cache2) = 0; 
};


class FeatureHist : public FeatureCompute {
  public:
    FeatureHist(int num_bins_, const std::vector<double>& thresholds_) : num_bins(num_bins_), thresholds(thresholds_) {} 
  
    void * create_cache()
    {
        return (void*)(new HistCache(num_bins+1));
    }
    
    void delete_cache(void * cache)
    {
        delete (HistCache*)(cache);
    }

    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
    {
        HistCache * hist_cache = (HistCache*) cache;
        ++(hist_cache->hist[val * num_bins]);
        ++(hist_cache->count);
    }
    
    void get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge<Label>* edge, unsigned int node_num)
    {
        HistCache * hist_cache = (HistCache*) cache;
        for (unsigned int i = 0; i < thresholds.size(); ++i) {
            feature_array.push_back(get_data(hist_cache, thresholds[i]));
        }
    } 

    // ?? difference feature -- not implemented because I am not returning histogram for now
    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge<Label>* edge)
    {
        throw ErrMsg("Not implemented");
    } 


    void merge_cache(void * cache1, void * cache2)
    {
        HistCache * hist_cache1 = (HistCache*) cache1;
        HistCache * hist_cache2 = (HistCache*) cache2;

        hist_cache1->count += (hist_cache2->count);
        for (int i = 0; i <= num_bins; ++i) {
            hist_cache1->hist[i] += hist_cache2->hist[i];
        }
        delete hist_cache2;
    }

  private:
    double get_data(HistCache * hist_cache, double threshold)
    {
        double threshold_amount = hist_cache->count * (threshold);
 
        hist_cache->hist[num_bins-1] += (hist_cache->hist[num_bins]);
        hist_cache->hist[num_bins] = 0;

        unsigned long long curr_count = 0;
        int spot = 0;
        unsigned long long cumval = 0;
        for (int i = 0; i < num_bins; ++i) {
            curr_count += hist_cache->hist[i];
            if (curr_count >= threshold_amount) {
                spot = i;
                break;
            }
            cumval += hist_cache->hist[i];
        }

        unsigned long long val2 = hist_cache->hist[spot];
        double slope = (curr_count - cumval);
        double median_spot = (threshold_amount - cumval)/slope;
        return ((median_spot + spot)/num_bins);
    }
        
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
  
    void * create_cache()
    {
        return (void*)(new MomentCache(num_moments));
    }
    
    void delete_cache(void * cache)
    {
        delete (MomentCache*)(cache);
    }

    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
    {
        MomentCache * moment_cache = (MomentCache*) cache;
        moment_cache->count += 1;
        for (int i = 0; i < num_moments; ++i) {
            moment_cache->vals[i] += std::pow(val, i+1);
        } 
    }
    
    void get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge<Label>* edge, unsigned int node_num)
    {
        MomentCache * moment_cache = (MomentCache*) cache;
        get_data(moment_cache, feature_array);
    } 

    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge<Label>* edge)
    {
        std::vector<double> vals1;
        std::vector<double> vals2;
        MomentCache * moment_cache1 = (MomentCache*) cache1;
        MomentCache * moment_cache2 = (MomentCache*) cache2;
        get_data(moment_cache1, vals1);
        get_data(moment_cache2, vals2);

        for (int i = 0; i < num_moments; ++i) {
            feature_array.push_back(std::abs(vals1[i] - vals2[i]));
        }
    } 

    void merge_cache(void * cache1, void * cache2)
    {
        MomentCache * moment_cache1 = (MomentCache*) cache1;
        MomentCache * moment_cache2 = (MomentCache*) cache2;

        moment_cache1->count += moment_cache2->count;
        for (int i = 0; i < num_moments; ++i) {
            moment_cache1->vals[i] += moment_cache2->vals[i];
        }
        delete moment_cache2;
    }

  private:
    void get_data(MomentCache * moment_cache, std::vector<double>& feature_array)
    {
        double count = double(moment_cache->count);
        //feature_array.push_back(count);
      
        // mean 
        double mean;
        if (num_moments > 0) {
            mean = moment_cache->vals[0] / count;
            feature_array.push_back(mean);
        }

        // variance
        double var;
        if (num_moments > 1) {
            var = moment_cache->vals[1] / count;
            double var_final = var - std::pow(mean, 2.0);
            feature_array.push_back(var_final);
        } 
    
        // skewness    
        double skew;
        if (num_moments > 2) {
            skew = moment_cache->vals[2] / count;
            double skew_final = skew - 3*mean*var + 2*std::pow(mean, 3.0);
            feature_array.push_back(skew_final);
        } 

        // kurtosis
        if (num_moments > 3) {
            double kurt = moment_cache->vals[3] / count;
            double kurt_final = kurt - 4*mean*skew + 6*std::pow(mean, 2.0)*var - 3*std::pow(mean, 4.0);
            feature_array.push_back(kurt_final);
        } 
    }
        
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
    
    void get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge<Label>* edge, unsigned int node_num)
    {
        RagNode<Label>* node1 = edge->get_node1();
        RagNode<Label>* node2 = edge->get_node2();

        if (node2->get_size() < node1->get_size()) {
            RagNode<Label>* temp_node = node2;
            node2 = node1;
            node1 = temp_node;
        }            
       
        if (node_num == 1) {
            get_node_features(node1, feature_array);
            return;
        } else if (node_num == 2) {
            get_node_features(node2, feature_array);
            return;
        }

        std::set<unsigned long long> lengths1;
        std::set<unsigned long long> lengths2;
        
        unsigned long long tot1 = get_lengths(node1, lengths1); 
        unsigned long long tot2 = get_lengths(node2, lengths2); 
        unsigned long long edge_tot = edge->get_size();

        double f1 = double(edge_tot)/tot1;
        double f2 = double(edge_tot)/tot2;

        if (f1 < f2) {
            feature_array.push_back(f1);
            feature_array.push_back(f2);
        } else {
            feature_array.push_back(f2);
            feature_array.push_back(f1);
        }

        double fm1 = double(edge_tot)/(*(lengths1.rbegin()));
        double fm2 = double(edge_tot)/(*(lengths2.rbegin()));

        if (fm1 < fm2) {
            feature_array.push_back(fm1);
            feature_array.push_back(fm2);
        } else {
            feature_array.push_back(fm2);
            feature_array.push_back(fm1);
        }
    } 

    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge<Label>* edge)
    {
        RagNode<Label>* node1 = edge->get_node1();
        RagNode<Label>* node2 = edge->get_node2();

        if (node2->get_size() < node1->get_size()) {
            RagNode<Label>* temp_node = node2;
            node2 = node1;
            node1 = temp_node;
        }            
        std::vector<double> temp_features1;
        std::vector<double> temp_features2;

        get_node_features(node1, temp_features1);
        get_node_features(node2, temp_features2);
    
        feature_array.push_back(temp_features1[0]-temp_features2[0]);
        feature_array.push_back(temp_features1[1]-temp_features2[1]);
    } 

    void merge_cache(void * cache1, void * cache2)
    {
        return;
    }

  private:
    unsigned long long get_lengths(RagNode<Label>* node, std::set<unsigned long long>& lengths)
    {
        unsigned long long count = 0;
        for (RagNode<Label>::edge_iterator iter = node->edge_begin(); iter != node->edge_end(); ++iter) {
            if ((*iter)->is_false_edge()) {
                continue;
            }
            count += (*iter)->get_size();
            lengths.insert((*iter)->get_size()); 
        }
        count += (node->get_border_size());
        lengths.insert(node->get_border_size());
        return count;
    }

    void get_node_features(RagNode<Label>* node, std::vector<double>& features)
    {
        std::set<unsigned long long> lengths;
        unsigned long long tot = get_lengths(node, lengths);
        
        std::set<unsigned long long>::reverse_iterator iter = lengths.rbegin();
        unsigned long long max_val = *iter;
        ++iter;
        unsigned long long second_max_val = *iter;

        features.push_back(double(max_val)/tot);
        features.push_back(double(second_max_val)/max_val);
    }


};


class FeatureCount : public FeatureCompute {
  public:
    FeatureCount() {} 
    void * create_cache()
    {
        return (void*)(new CountCache());
    }
    
    void delete_cache(void * cache)
    {
        delete (CountCache*)(cache);
    }

    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
    {
        CountCache * count_cache = (CountCache*) cache;
        count_cache->count += 1;
    }
    
    void get_feature_array(void* cache, std::vector<double>& feature_array, RagEdge<Label>* edge, unsigned int node_num)
    {
        CountCache * count_cache = (CountCache*) cache;
        feature_array.push_back(count_cache->count);
    } 

    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array, RagEdge<Label>* edge)
    {
        CountCache * count_cache1 = (CountCache*) cache1;
        CountCache * count_cache2 = (CountCache*) cache2;

        feature_array.push_back(std::abs(count_cache1->count - count_cache2->count));
    } 

    void merge_cache(void * cache1, void * cache2)
    {
        CountCache * count_cache1 = (CountCache*) cache1;
        CountCache * count_cache2 = (CountCache*) cache2;

        count_cache1->count += count_cache2->count;
        delete count_cache2;
    }
};




}




#endif
