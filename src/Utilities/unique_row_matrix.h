#ifndef _unique_row_matrix
#define _unique_row_matrix

#include <set>
#include <vector>
#include <cstdio>
#include <cmath>
#include <cfloat>

using namespace std;

struct vectorcomp {
    bool operator() (const vector<double>& lhs, const vector<double>& rhs) const
    {
        for(size_t i = 0 ; i < lhs.size()-1 ; i++){
            if ( fabs(lhs[i] - rhs[i]) > DBL_EPSILON){
                if (lhs[i] > rhs[i])
                    return false; 
                else
                    return true; 	
            }
        }
        return false;

    }
};


class UniqueRowMatrix {
  public:	
    UniqueRowMatrix(): _nrows(0), _ncols(0)
    {
        _rows.clear();
    }	

    int insert(const vector<double>& newrow)
    {
        size_t prev_nrows = _nrows;
        if (_ncols>0 && _ncols != newrow.size()){
            printf("vector size mismatch\n");
            return 0;
        }
        _rows.insert(newrow);
        _nrows = _rows.size();
        _ncols = (_rows.begin())->size();
        if (_nrows > prev_nrows)
            return 1;
        else return 0;
    }	
   
    void get_matrix(vector< vector<double> >& rmatrix)
    {
        rmatrix.clear(); 
        rmatrix.resize(_nrows); 

        set< vector<double>, vectorcomp >::iterator sit;
        int rr=0;
        for(sit = _rows.begin(); sit != _rows.end(); sit++, rr++){
            rmatrix[rr].resize(_ncols);	
            for(size_t j = 0; j< sit->size(); j++ )
                rmatrix[rr][j] = sit->at(j);
        }
    }	

    void get_vector( vector<double>& rvector)
    {
        rvector.clear(); 
        rvector.resize(_nrows*_ncols);

        set< vector<double>, vectorcomp >::iterator sit;
        size_t rr=0;
        for(sit = _rows.begin(); sit != _rows.end(); sit++){
            for(size_t j = 0; j< sit->size(); j++, rr++ )
                rvector[rr] = sit->at(j);
        }
    }	

    virtual void print_matrix()
    {
        set< vector<double>, vectorcomp >::iterator sit;
        for(sit = _rows.begin(); sit != _rows.end(); sit++){
            for(size_t j=0; j< sit->size(); j++)
                printf("%lf ", sit->at(j));
            printf("\n");
        }	
    }

    size_t nrows()
    {
        return _nrows;
    }

    void clear()
    {
        _rows.clear();
        _nrows = 0;
        _ncols = 0;
    }

  protected:
    set< vector<double>, vectorcomp > _rows ;
    size_t _nrows;
    size_t _ncols;	

  
  private:
    set< vector<double>, vectorcomp >& get_set(){return _rows;};	
};

class UniqueRowFeature_Label: public UniqueRowMatrix{

public:

    void get_feature_label(vector< vector<double> >& rmatrix, vector<int>& rlabels){
	rmatrix.clear(); rlabels.clear();
	rmatrix.resize(_nrows); rlabels.resize(_nrows);

	set< vector<double>, vectorcomp >::iterator sit;
	int rr=0;
	for(sit = _rows.begin(); sit != _rows.end(); sit++, rr++){
	    //rmatrix.push_back(*sit);	
	    rmatrix[rr].resize(_ncols-1);	
	    for(size_t j = 0; j< sit->size()-1; j++ )
		rmatrix[rr][j] = sit->at(j);
	    rlabels[rr] = (int) sit->at(sit->size()-1);	
	}
    }	


}; 

class UniqueRowMatrix_Chull: public UniqueRowMatrix {
  public:
    int insert(const vector<double>& newrow){

	if (ux.size()<2)
	    ux.insert(newrow[0]);	
	if (uy.size()<2)
	    uy.insert(newrow[1]);	
	if (uz.size()<2)
	    uz.insert(newrow[2]);	

	UniqueRowMatrix::insert(newrow);
    }

    bool is_valid(int nDim){
	// *C* check single plane
        if ( (ux.size()< 2) || (uy.size()< 2) || (uz.size()< 2) )
 	    return false;

	// *C* check degeneracy
	if (_nrows< (nDim+1))	
	    return false;

	return true;
	
    }	
	
    void append(UniqueRowMatrix_Chull& another){
	set< vector<double>, vectorcomp >::iterator sit;
	set< vector<double>, vectorcomp >& another_set = another._rows;
	for(sit = another_set.begin(); sit != another_set.end(); sit++)
	    insert(*sit);		
    }

  private:
    set<double> ux;
    set<double> uy;
    set<double> uz;


};
#endif
