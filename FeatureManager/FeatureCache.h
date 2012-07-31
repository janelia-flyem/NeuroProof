#ifndef FEATURECACHE_H
#define FEATURECACHE_H

namespace NeuroProof {

struct MomentCache {
    MomentCache(unsigned int num_moments) : count(0), vals(num_moments, 0) {}
    unsigned long long count; 
    vector<double> vals;
};

struct HistCache {
    HistCache(unsigned num_bins) : count(0), hist(num_bins, 0) {}
    unsigned long long count;
    vector<unsigned long long> hist;
};

}

#endif


