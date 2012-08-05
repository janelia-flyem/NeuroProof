#ifndef FEATURES_H
#define FEATURES_H

#include "FeatureCache.h"
#include "ErrMsg.h"

namespace NeuroProof {

class FeatureCompute {
  public:
    virtual void * create_cache() = 0;
    virtual void delete_cache(void * cache) = 0;
    virtual void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0) = 0;
    virtual void  get_feature_array(void* cache, std::vector<double>& feature_array) = 0; 
    virtual void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array) = 0; 
    // will delete second cache
    void merge_cache(void * cache1, void * cache2) = 0; 
};


class FeatureHist : public FeatureCompute {
  public:
    FeatureHist(int num_bins_, const std::vector<double>& thresholds_) : num_bins(num_bins_), thresholds(thresholds_) {} 
  
    void void * create_cache()
    {
        return (void*)(new HistCache(num_bins));
    }
    
    void delete_cache(void * cache)
    {
        delete (HistCache*)(cache);
    }

    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
    {
        HistCache * hist_cache = (HistCache*) cache;
        ++(hist_cache->hist[val * num_bins]);
        ++(hist_cache->count)
    }
    
    void get_feature_array(void* cache, std::vector<double>& feature_array)
    {
        HistCache * hist_cache = (HistCache*) cache;
        for (unsigned int i = 0; i < thresholds.size(); ++i) {
            feature_array.push_back(get_data(hist_cache, thresholds[i]));
        }
    } 

    // ?? difference feature -- not implemented because I am not returning histogram for now
    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array)
    {
        throw ErrMsg("Not implemented")
    } 


    void merge_cache(void * cache1, void * cache2)
    {
        HistCache * hist_cache1 = (HistCache*) cache1;
        HistCache * hist_cache2 = (HistCache*) cache2;

        hist_cache1->count += (hist_cache2->count);
        for (int i = 0; i < num_bins; ++i) {
            hist_cache1->hist[i] += hist_cache2->hist[i];
        }
        delete hist_cache2;
    }

  private:
    double get_data(HistCache * hist_cache, double threshold)
    {
        double threshold_amount = int(hist_cache->count * (threshold));
    
        int curr_count = 0;
        int spot = 0;
        for (int i = 0; i < num_bins; ++i) {
            curr_count += hist_cache->hist[i];
            if (curr_count >= threshold_amount) {
                spot = i;
            }
        }
       
        unsigned long long val1 = 0;
        if (spot > 0) {
            val1 = hist_cache->hist[spot-1];
        }
        unsigned long long val2 = hist_cache->hist[spot];
        double incr = 1.0/num_bins;
        double slope = (val2 - val1) / incr; 
        double median_spot = (threshold_amound - (spot*incr))/slope;
        return (median_spot + spot*incr);
    }
        
    int num_bins;
    std::vector<double> thresholds; 
};

// !! temporary support only 0, 1, 2, 3, and 4
class FeatureMoment : public FeatureCompute {
  public:
    FeatureMoment(int num_moments_) : num_moments(num_moments_)
    {
        assert((num_moments <= 4) && (num_moments >= 0));
    } 
  
    void void * create_cache()
    {
        return (void*)(new MomentCache(num_bins));
    }
    
    void delete_cache(void * cache)
    {
        delete (MomentCache*)(cache);
    }

    void add_point(double val, void * cache, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
    {
        MomentCache * moment_cache = (MomentCache*) cache;
        moment_cache->count += 1;
        for (int i = 1; i <= num_moments; ++i) {
            moments_cache->vals[i-1] += std::pow(val, double(i));
        } 
    }
    
    void get_feature_array(void* cache, std::vector<double>& feature_array)
    {
        MomentCache * moment_cache = cache;
        get_data(moment_cache, feature_array);
    } 

    void  get_diff_feature_array(void* cache2, void * cache1, std::vector<double>& feature_array)
    {
        std::vector<double> vals1;
        std::vector<double> vals2;
        MomentCache * moment_cache1 = (MomentCache*) cache1;
        MomentCache * moment_cache2 = (MomentCache*) cache2;
        get_data(moment_cache1, vals1);
        get_data(moment_cache2, vals2);

        for (int i = 0; i <= num_moments; ++i) {
            feature_array.push_back(std::abs(vals1[i] - vals2[i]));
        }
    } 

    void merge_cache(void * cache1, void * cache2)
    {
        MomentCache * moment_cache1 = (MomentCache*) cache1;
        MomentCache * moment_cache2 = (MomentCache*) cache2;

        for (int i = 0; i <= num_moments; ++i) {
            moment_cache1->vals[i] += moment_cache2->vals[i];
        }
        delete moment_cache2;
    }

  private:
    void get_data(MomentCache * moment_cache, std::vector<double>& feature_array)
    {
        double count = double(moment_cache->count);
        feature_array.push_back(count);
      
        // mean 
        double mean;
        if (num_moments > 0) {
            mean = moment_cache->vals[0] / count;
            feature_array.push_back(mean/count);
        }

        // variance
        double var;
        if (num_moments > 1) {
            double var = moment_cache->vals[1] / count;
            double var_final = var - std::pow(mean, 2.0);
            featuer_array.push_back(var_final);
        } 
    
        // skewness    
        double skew;
        if (num_moments > 2) {
            double skew = moment_cache->vals[2] / count;
            double skew_final = sket - 3*mean*var + 2*std::pow(mean, 3.0);
            featuer_array.push_back(skew_final);
        } 

        // kurtosis
        double kurt;
        if (num_moments > 3) {
            double kurt = moment_cache->vals[3] / count;
            double kurt_final = kurt - 4*mean*skew + 6*std::pow(mean, 2.0)*var - 3*std::pow(mean, 4.0);
            featuer_array.push_back(kurt_final);
        } 
    }
        
    unsigned int num_moments;
};


}




#endif
