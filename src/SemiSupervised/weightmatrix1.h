#ifndef _WEIGHT_MATRIX1
#define _WEIGHT_MATRIX1

#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <boost/thread.hpp>
#include <boost/ref.hpp>
#include "solve_amg.hpp"

namespace NeuroProof{
  

class RowVal1{

public:
  
    unsigned int j;
    double v;
  
    RowVal1(unsigned int pj, double pv): j(pj), v(pv) {};
};
class WeightMatrix1{

    
    std::vector< std::vector<RowVal1> > _wtmat;
    std::vector< std::vector<RowVal1> > _Wul;
    std::vector< std::vector<RowVal1> > _Luu;
    
    std::set<unsigned int> _ignore;
    

    std::vector<int> _Luu_i;
    std::vector<int> _Luu_j;
    std::vector<double> _Luu_v;
    
    
    unsigned int _n;
 
    std::vector<double> _deltas;
    size_t _nrows;
    size_t _ncols;
    
    size_t _nlabeled;
    
    double _thd;
    
    std::vector<double> _degree;
    
    std::vector<int> _trn_map; //faster than multimap
    std::vector<int> _trn_lbl; 
    std::vector<int> _tst_map;

    
    size_t _trn_count;
    size_t _tst_count;
    
    AMGsolver amgsolver;
    
public:
  
    WeightMatrix1(double pthd, std::vector<unsigned int> &pignore): _thd(pthd)
    { 
	_trn_map.clear(); _trn_count = 0;
	_trn_lbl.clear(); 
	_tst_map.clear(); _tst_count = 0;
	
	for(size_t ii=0; ii < pignore.size(); ii++ )
	    _ignore.insert(pignore[ii]);
	
    }; 
    
    
    void weight_matrix(std::vector< std::vector<double> >& pfeatures,
		 bool exhaustive);
    void weight_matrix_parallel(std::vector< std::vector<double> >& pfeatures,
		 bool exhaustive);

    void compute_weight_partial(std::map<double, unsigned int>& sorted_features_part,
			    std::map<double, unsigned int>& sorted_features,
			    std::vector< std::vector<double> >& pfeatures,
			    std::vector< std::vector<RowVal1> >& wtmat,
			    std::vector<double>& degree);
    
    
    void EstimateBandwidth(std::vector< std::vector<double> >& pfeatures, 
			   std::vector<double>& deltas);
    double distw(std::vector<double>& vecA, std::vector<double>& vecB);
    double vec_dot(std::vector<double>& vecA, std::vector<double>& vecB);
    void scale_vec(std::vector<double>& vecA, std::vector<double>& deltas);
    double vec_norm(std::vector<double>& vecA);
    
    void find_nonzero_degree(std::vector<unsigned int>& ridx);
    void find_large_degree(std::vector<unsigned int>& ridx);
    
    void compute_Wul_Luu();
    void LSsolve(std::map<unsigned int, double>& result);
    void AMGsolve(std::map<unsigned int, double>& result);
    void compute_rhs(double *rhs);
    void add2trnset(std::vector<unsigned int>& trnset, std::vector<int> &labels);
    
    void copy(std::vector< std::vector<double> >& src, std::vector< std::vector<double> >& dst);
    void scale_features(std::vector< std::vector<double> >& allfeatures,
				    std::vector< std::vector<double> >& pfeatures);
    double nnz_pct();
  
};


}
#endif
