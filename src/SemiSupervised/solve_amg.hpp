#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <utility>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>

#include <amgcl/amgcl.hpp>
#include <amgcl/interp_smoothed_aggr.hpp>
#include <amgcl/aggr_plain.hpp>
#include <amgcl/level_cpu.hpp>
#include <amgcl/operations_ublas.hpp>
#include <amgcl/cg.hpp>
//#include <amgcl/profiler.hpp>

//#include "read.hpp"

typedef boost::numeric::ublas::compressed_matrix<double, boost::numeric::ublas::row_major> ublas_matrix;
typedef boost::numeric::ublas::vector<double> ublas_vector;
typedef std::vector<double> rvector;
typedef std::vector<int> ivector;


// namespace amgcl {
//     profiler<> prof("ublas");
// }
// using amgcl::prof;
typedef amgcl::solver<
    double, ptrdiff_t,
    amgcl::interp::smoothed_aggregation<amgcl::aggr::plain>,
    amgcl::level::cpu<amgcl::relax::spai0>
    > AMG;


class AMGsolver{
  
    ublas_matrix* A;
    ublas_vector* rhs;
    int n;
    
    AMG* amg;
    
public:
  
    AMGsolver(){
	A=NULL; rhs=NULL; amg=NULL;
	n=0;
    }
  
    void set_size(int pn){n = pn; };
    
    void set_rhs(rvector &prhs){
	rhs = new ublas_vector();
	rhs->resize(prhs.size());
	for (int ii=0; ii < prhs.size(); ii++)
	    (*rhs)[ii] = prhs[ii];
	n = rhs->size();
      
    }
    
    void set_matrix(int nz, ivector &row, ivector &col, rvector &val){
      
	if(n==0){
	    printf("matrix dimension ==0 \n");
	    return;
	}
	A = new ublas_matrix(n, n);
	A->reserve(nz);
	
	size_t ii=0;
	for (int rr=0; rr < n; rr++){
	    rvector vmarker(n,0);
	    while((row[ii] == rr) &&  (ii < nz)){
		vmarker[col[ii]] = val[ii];
		ii++;
	    }
	    for(int cc=0; cc<n ; cc++){
		if (vmarker[cc] > 0 || vmarker[cc] < 0)
		    A->push_back(rr, cc, vmarker[cc]);
	    }
	}
    }
  

    void build_preconditioner(){
	// Build the preconditioner:
	if (n==0 || A==NULL){
	    printf("Matrix in AMG solver is not initialized.\n");
	    return;
	}

	// Use K-Cycle on each level to improve convergence:
	AMG::params prm;
	prm.level.kcycle = 1;

	//prof.tic("setup");
	amg = new AMG( amgcl::sparse::map(*A), prm );
	//prof.toc("setup");
    }
  
    void solveAMG(int nz, ivector &row, ivector &col, rvector &val, rvector &prhs, rvector &result) 
    {
	// Create ublas matrix with the data.
	
	set_rhs(prhs);
	set_matrix(nz, row, col, val);

	build_preconditioner();

	//std::cout << amg << std::endl;

	// Solve the problem with CG method. Use AMG as a preconditioner:
	ublas_vector x(n, 0);
	//prof.tic("solve (cg)");
	std::pair<int,double> cnv = amgcl::solve(*A, *rhs, *amg, x, amgcl::cg_tag());
	//prof.toc("solve (cg)");

	result.resize(x.size());
	for(int ii=0; ii<x.size(); ii++)
	    result[ii] = x[ii];
	
	
	std::cout << "Error:      " << cnv.second << "  "
		  << "Iterations: " << cnv.first  << std::endl;

	release_var();	  
	//std::cout << prof;
    }
    void solveAMG( rvector &prhs, rvector &result) 
    {
	
	if ( (n != prhs.size()) || A==NULL){
	    printf("AMG solver is not initialized.\n");
	    return;
	}
	set_rhs(prhs);

	// Solve the problem with CG method. Use AMG as a preconditioner:
	ublas_vector x(n, 0);
	//prof.tic("solve (cg)");
	std::pair<int,double> cnv = amgcl::solve(*A, *rhs, *amg, x, amgcl::cg_tag());
	//prof.toc("solve (cg)");

	result.resize(x.size());
	for(int ii=0; ii<x.size(); ii++)
	    result[ii] = x[ii];
	
	
	std::cout << "Error:      " << cnv.second << "  "
		  << "Iterations: " << cnv.first  << std::endl;
		  
	release_var();	  
	//std::cout << prof;
    }
    
    void release_var(){
	n=0;
	if (A!=NULL){
	    delete A;
	    A = NULL;
	}
	if (amg!=NULL){
	    delete amg;
	    amg = NULL;
	}
	if (rhs!=NULL){
	    delete rhs;
	    rhs = NULL;
	}
    }
};






