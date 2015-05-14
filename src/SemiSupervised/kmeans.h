#ifndef _KMEANS_
#define _KMEANS_

#include <float.h>
#include <memory.h>
#include <vector>
#include <cstdio>
#include <algorithm>

namespace NeuroProof{
  
class kMeans{

    unsigned int _K;
    unsigned int _maxIter;
    double _minDelta;

    size_t _ndata;
    size_t _dim;
    
public:
    
    kMeans(unsigned int pK, unsigned int pmaxIter, double pmindelta):_K(pK), _maxIter(pmaxIter), _minDelta(pmindelta){};

    void compute_centers(std::vector< std::vector<double> >& data, std::vector<unsigned int>& cidx);
    void initial_centers(std::vector< std::vector<double> >& data, std::vector< std::vector<double> >& ctrs);
    double kmiter(std::vector< std::vector<double> >& data, std::vector< std::vector<double> >& ctrs, std::vector<unsigned int>& idx_closest_pt);
  
};

}
#endif
