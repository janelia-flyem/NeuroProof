#include "weightmatrix1.h"

#define EPS 1e-6

using namespace NeuroProof;

double WeightMatrix1::vec_dot(std::vector<double>& vecA, std::vector<double>& vecB)
{
    double dd=0;
    size_t ll=0;
    for(size_t j = 0; j < vecA.size(); j++){
	double d1 = (vecA[j] * vecB[j]);
	ll++;
	dd += d1;
    }
    return dd;
}


void WeightMatrix1::compute_Wul_Luu()
{
    

    _Wul.clear(); _Wul.resize(_tst_count);
    _Luu.clear(); _Luu.resize(_tst_count);
    _Luu_i.clear(); _Luu_j.clear(); _Luu_v.clear();
    for(size_t ii = 0 ; ii < _tst_map.size() ; ii++){
	  int mapped_idx_u = _tst_map[ii];
	  if (mapped_idx_u == -1)
	      continue;
	  
	  _Luu[mapped_idx_u].push_back(RowVal1(mapped_idx_u, _degree[ii]));
	  _Luu_i.push_back(mapped_idx_u); _Luu_j.push_back(mapped_idx_u); _Luu_v.push_back(_degree[ii]);
	  
	  for(size_t jj=0; jj < _wtmat[ii].size(); jj++){
	      unsigned int col = _wtmat[ii][jj].j;
	      double val = _wtmat[ii][jj].v;
	      if (_trn_map[col] != -1){ // labeled pt
		  //add to Wul
		  unsigned int mapped_idx_l = _trn_map[col];
		  _Wul[mapped_idx_u].push_back(RowVal1(mapped_idx_l, val));
	      }
	      else if (_tst_map[col] != -1){
		  // add to Luu
		  unsigned int mapped_idx_u2 = _tst_map[col];
		  _Luu[mapped_idx_u].push_back(RowVal1(mapped_idx_u2, -val));
		  
		  _Luu_i.push_back(mapped_idx_u); _Luu_j.push_back(mapped_idx_u2); _Luu_v.push_back(-val);
		  
	      }
	  }
    }
}

void WeightMatrix1::LSsolve(std::map<unsigned int, double>& result)
{
  
}

void WeightMatrix1::AMGsolve(std::map<unsigned int, double>& result)
{
  
    std::vector<double> rhs(_tst_count);
    compute_rhs(rhs.data());
    std::vector<double> x(_tst_count,0);

    /*C* debug
    FILE* fp= fopen("semi-supervised/check_Luu.txt","wt");
    for(size_t ii=0; ii <_Luu_i.size(); ii++)
	fprintf(fp,"%u %u %lf\n",_Luu_i[ii], _Luu_j[ii], _Luu_v[ii]);
    fclose(fp);
    
    fp= fopen("semi-supervised/check_rhs.txt","wt");
    for(size_t ii=0; ii < rhs.size(); ii++)
	fprintf(fp,"%lf\n", rhs[ii]);
    fclose(fp);
    //C*/
    
    /*C* debug*
    if(rhs.size() != _Wul.size())
	printf("\n rhs size not equal to Wul size \n\n");
    /**/  
    
    //amgsolver.solveAMG(_Luu_i.size(), _Luu_i, _Luu_j, _Luu_v, rhs, x);
    amgsolver.solveAMG(rhs, x);
    
    
    result.clear();
    for(size_t ii=0; ii < _nrows; ii++){
	if (_tst_map[ii] != -1){
	    unsigned int mapped_idx = _tst_map[ii];
	    result.insert(std::make_pair(ii, x[mapped_idx]));
	}
    }  
}

void WeightMatrix1::compute_rhs(double *rhs)
{
  
    for(size_t ii=0; ii < _Wul.size(); ii++){
	for(size_t jj=0; jj < _Wul[ii].size(); jj++){
	    unsigned int col = _Wul[ii][jj].j;
	    double val = _Wul[ii][jj].v;
	    
	    (rhs[ii])  += (_trn_lbl[col]*val);
	}
    }
}
void WeightMatrix1::add2trnset(std::vector<unsigned int>& trnset, std::vector<int> &trnlabels)
{
  
    _trn_map.clear(); _trn_count = 0; _trn_lbl.clear();
    _trn_map.resize(_nrows,-1);
    for(size_t ii=0; ii < trnset.size(); ii++){
	unsigned int idx = trnset[ii];
	if (_degree[idx]>0){
	    _trn_map[idx] = _trn_count++;
	    _trn_lbl.push_back(trnlabels[ii]);
// 	    _trn_lbl.push_back(alllabels[idx]);
	}
    }
    _tst_map.clear(); _tst_count = 0;
    _tst_map.resize(_nrows,-1);
    for(size_t ii=0; ii < _nrows; ii++){
	if ( (_degree[ii]>0) && (_trn_map[ii] == -1) ) // degree > 0 and not in trn set
	    _tst_map[ii] = _tst_count++;
    }
    
    compute_Wul_Luu();
    
    amgsolver.set_size(_Wul.size());
    
    std::vector<double> Luu_v2(_Luu_v.size(),0);
    for(size_t ii=0; ii < _Luu_i.size(); ii++){
	if(_Luu_i[ii] == _Luu_j[ii])
	  Luu_v2[ii] = _Luu_v[ii]+0.0001;
	else
	  Luu_v2[ii] = _Luu_v[ii];
    }
    
    
    amgsolver.set_matrix(_Luu_i.size(), _Luu_i, _Luu_j, Luu_v2);
    
    
//     amgsolver.set_matrix(_Luu_i.size(), _Luu_i, _Luu_j, _Luu_v);
    amgsolver.build_preconditioner();
}







//*************************************************************************************************
void WeightMatrix1::copy(std::vector< std::vector<double> >& src, std::vector< std::vector<double> >& dst)
{
    size_t nrows = src.size();
    size_t ncols = src[0].size() - _ignore.size();
    
    dst.clear();
    dst.resize(nrows);
    for(size_t rr=0; rr < nrows; rr++){
	dst[rr].resize(ncols);
	for(size_t cc=0, cc1=0; cc < src[rr].size(); cc++){
	    if ( (cc1 < ncols) && (_ignore.find(cc) == _ignore.end())){
	      if (fabs(src[rr][cc])>EPS) //for numerical stability
		  dst[rr][cc1] = src[rr][cc]; 
	      else
		  dst[rr][cc1] = 0; 
	      cc1++;
	    }
	}
    }
    
}
void WeightMatrix1::EstimateBandwidth(std::vector< std::vector<double> >& pfeatures,
				      std::vector<double>& deltas)
{
    deltas.clear();
    deltas.resize(_ncols);
    for(size_t col=0; col< _ncols; col++){
	// *C* compute mean
	double mean= 0;
	for(size_t row = 0; row < _nrows; row++){
	    mean += pfeatures[row][col];
	}
	mean /= _nrows;
	
	// *C* compute std dev
	double sqdev = 0;
	for(size_t row=0; row < _nrows; row++){
	    sqdev += ((pfeatures[row][col] -mean)*(pfeatures[row][col] -mean));
	}
	sqdev /= _nrows;
	
	deltas[col] = sqrt(sqdev);
    }
  
}

double WeightMatrix1::distw(std::vector<double>& vecA, std::vector<double>& vecB)
{
    double dd=0;
//     size_t ll=0;
    for(size_t j = 0; j < vecA.size(); j++){
	if (_deltas[j]>EPS){
	    double d1 = (vecA[j] - vecB[j])*(vecA[j] - vecB[j]);
	    double d2 = d1 / (_deltas[j]*_deltas[j]);
// 	    ll++;
	    dd += d2;
	}
    }
    //dd /= ll;
    return dd;
}
// double WeightMatrix1::vec_dot(std::vector<double>& vecA, std::vector<double>& vecB)
// {
//     double dd=0;
//     size_t ll=0;
//     for(size_t j = 0; j < vecA.size(); j++){
// 	double d1 = (vecA[j] * vecB[j]);
// 	ll++;
// 	dd += d1;
//     }
//     return dd;
// }

void WeightMatrix1::scale_vec(std::vector<double>& vecA, std::vector<double>& deltas)
{
  
    for(size_t j=0; j< vecA.size(); j++){
	if (deltas[j]>EPS){
	    vecA[j] /= deltas[j];
	}
    }
       
}

double WeightMatrix1::vec_norm(std::vector<double>& vecA)
{
    double nrm = 0;
    for(size_t j=0; j< vecA.size(); j++){
	nrm += (vecA[j]*vecA[j]);
    }
    return nrm;   
}

void WeightMatrix1::find_nonzero_degree(std::vector<unsigned int>& ridx)
{
  
    ridx.clear();
    
    std::vector<double> degree(_nrows,0);
    
    for(size_t rr = 0; rr < _wtmat.size(); rr++){
	if (_degree[rr]>0){
	    ridx.push_back(rr);
	}
    }
}
void WeightMatrix1::find_large_degree(std::vector<unsigned int>& ridx){
  
    std::multimap<double, unsigned int> sorted_degree;
    for(size_t ii=0; ii < _nrows ; ii++){
	sorted_degree.insert(std::make_pair(_degree[ii], ii));
    }
    
    std::vector<unsigned int> flag(_nrows, 0);
    std::multimap<double, unsigned int>::reverse_iterator sit = sorted_degree.rbegin();
    for(; sit!= sorted_degree.rend(); sit++){
	unsigned int sidx = sit->second;
	if (!flag[sidx]){
	    flag[sidx] = 1;
	    ridx.push_back(sidx);
	    std::vector<RowVal1>& nbr_wts = _wtmat[sidx];
	    for(size_t ii=0; ii < nbr_wts.size(); ii++ ){
		unsigned int sjdx = nbr_wts[ii].j;
		flag[sjdx] = 2;
	    }
	}
	  
    }
    
}

void WeightMatrix1::scale_features(std::vector< std::vector<double> >& allfeatures,
				    std::vector< std::vector<double> >& pfeatures)
{
    copy(allfeatures, pfeatures);


    std::vector<double> deltas;
    EstimateBandwidth(pfeatures, deltas);
    for(size_t i=0; i < pfeatures.size(); i++){
	std::vector<double>& tmpvec = pfeatures[i];
	scale_vec(tmpvec, deltas);
    }
}

void WeightMatrix1::weight_matrix(std::vector< std::vector<double> >& allfeatures,
			   bool exhaustive)
{

  
    std::vector< std::vector<double> > pfeatures;
    copy(allfeatures, pfeatures);

    _nrows = pfeatures.size();
    _ncols = pfeatures[0].size();

    EstimateBandwidth(pfeatures, _deltas);
    
    _wtmat.resize(_nrows);
    _degree.resize(_nrows, 0.0);
    
    unsigned long nnz=0;
   
    std::map<double, unsigned int> sorted_features;
    for(size_t i=0; i < pfeatures.size(); i++){
	std::vector<double>& tmpvec = pfeatures[i];
	scale_vec(tmpvec, _deltas);
	double nrm = vec_norm(tmpvec);
	sorted_features.insert(std::make_pair(nrm, i));
    }
    std::map<double, unsigned int>::iterator sit_i;    
    std::map<double, unsigned int>::iterator sit_j;    
    
    
    for(sit_i = sorted_features.begin() ; sit_i != sorted_features.end(); sit_i++){
	
	sit_j = sit_i;
	sit_j++;
	size_t i = sit_i->second;
	unsigned int nchecks = 0;
	for(; sit_j != sorted_features.end(); sit_j++){
	    size_t j = sit_j->second;
	    double dot_ij = vec_dot(pfeatures[i], pfeatures[j]);  
	    double dist = sit_i->first + sit_j->first - 2*dot_ij;
	    double min_dist = sit_i->first + sit_j->first - 2*sqrt(sit_i->first)*sqrt(sit_j->first);
	    
	    nchecks++;
	    
	    if (dist < (_thd*_thd)){
		double val =  exp(-dist/(0.5*_thd*_thd));
		
		_wtmat[i].push_back(RowVal1(j, val));
		(_degree[i]) += val;
		_wtmat[j].push_back(RowVal1(i, val));
		(_degree[j]) += val;
		
		nnz += 2;
	    }
	    if (min_dist>(_thd*_thd)){
	      break;
	    }
	}
    }
    printf("total nonzeros: %lu\n",nnz);

    /* C debug*
    FILE* fp=fopen("semi-supervised/tmp_wtmatrix.txt", "wt");
    for(size_t rr = 0; rr < _wtmat.size(); rr++){
	double luu = 0;
	double w_l = 0;
	for(size_t cc=0; cc < _wtmat[rr].size(); cc++){
	    size_t cidx = _wtmat[rr][cc].j;
	    double val = _wtmat[rr][cc].v;
	    //val = exp(-val/(0.5*_thd*_thd));
	    fprintf(fp, "%u %u %lf\n", rr, cidx, val);
	}
    }
    fclose(fp);
    //*C*/
    
    
}

void WeightMatrix1::weight_matrix_parallel(std::vector< std::vector<double> >& allfeatures,
			   bool exhaustive)
{

    /* C debug*
    FILE* fp=fopen("features.txt", "wt");
    for(size_t rr = 0; rr < allfeatures.size(); rr++){
	for(size_t cc=0; cc < allfeatures[rr].size(); cc++){
	    fprintf(fp, "%lf ", allfeatures[rr][cc]);
	}
	fprintf(fp,"\n");
    }
    fclose(fp);
    /*C*/
  
    printf("running parallel version\n");
    std::vector< std::vector<double> > pfeatures;
    copy(allfeatures, pfeatures);

    _nrows = pfeatures.size();
    _ncols = pfeatures[0].size();

    EstimateBandwidth(pfeatures, _deltas);
    
    _wtmat.resize(_nrows);
    _degree.resize(_nrows, 0.0);
    
    size_t NCORES = 8;
   
    std::map<double, unsigned int> sorted_features; // all features are unique, check learn or iterative learn algo
    std::vector< std::map<double, unsigned int> > sorted_features_p(NCORES); // all features are unique, check learn or iterative learn algo
    int part = 0;  
    for(size_t i=0; i < pfeatures.size(); i++){
	std::vector<double>& tmpvec = pfeatures[i];
	scale_vec(tmpvec, _deltas);
	double nrm = vec_norm(tmpvec);
	sorted_features.insert(std::make_pair(nrm, i));
	
	(sorted_features_p[part]).insert(std::make_pair(nrm, i));
	part = (part+1) % NCORES;
    }
    
    std::vector< boost::thread* > threads;
    std::vector< std::vector< std::vector<RowVal1> > > tmpmat(sorted_features_p.size());
    std::vector< std::vector<double> > degrees(sorted_features_p.size());
    for(size_t pp=0; pp < sorted_features_p.size(); pp++){
      tmpmat[pp].resize(_nrows);
      degrees[pp].resize(_nrows, 0.0);
      //compute_weight_partial(sorted_features_p[i], sorted_features, pfeatures);
      threads.push_back(new boost::thread(&WeightMatrix1::compute_weight_partial, this, sorted_features_p[pp], sorted_features, pfeatures, boost::ref(tmpmat[pp]), boost::ref(degrees[pp])));
    }
    
    printf("Sync all threads \n");
    for (size_t ti=0; ti<threads.size(); ti++) 
      (threads[ti])->join();
    printf("all threads done\n");
    
    for(size_t pp=0; pp < tmpmat.size(); pp++){
	for(size_t i=0; i < tmpmat[pp].size(); i++){
	    if(tmpmat[pp][i].size()>0){
	       _wtmat[i].insert(_wtmat[i].end(),tmpmat[pp][i].begin(), tmpmat[pp][i].end());
	       (_degree[i]) += degrees[pp][i];
	    }
	}
    }    
    
    /* C debug*
    FILE* fp=fopen("semi-supervised/tmp_wtmatrix_parallel.txt", "wt");
    for(size_t rr = 0; rr < _wtmat.size(); rr++){
	double luu = 0;
	double w_l = 0;
	for(size_t cc=0; cc < _wtmat[rr].size(); cc++){
	    size_t cidx = _wtmat[rr][cc].j;
	    double val = _wtmat[rr][cc].v;
	    //val = exp(-val/(0.5*_thd*_thd));
	    fprintf(fp, "%u %u %lf\n", rr, cidx, val);
	}
    }
    fclose(fp);
    //*C*/
    
    
}


void WeightMatrix1::compute_weight_partial(std::map<double, unsigned int>& sorted_features_part,
			    std::map<double, unsigned int>& sorted_features,
			    std::vector< std::vector<double> >& pfeatures,
			    std::vector< std::vector<RowVal1> >& wtmat,
			    std::vector<double>& degree)
{
  
    unsigned long nnz=0;
    std::map<double, unsigned int>::iterator sit_i;    
    std::map<double, unsigned int>::iterator sit_j;    
    for(sit_i = sorted_features_part.begin() ; sit_i != sorted_features_part.end(); sit_i++){
	
	size_t i = sit_i->second;
	double norm_i = sit_i->first;
	sit_j = sorted_features.find(norm_i);
	sit_j++;
	unsigned int nchecks = 0;
	for(; sit_j != sorted_features.end(); sit_j++){
	    size_t j = sit_j->second;
	    double dot_ij = vec_dot(pfeatures[i], pfeatures[j]);  
	    double dist = sit_i->first + sit_j->first - 2*dot_ij;
	    double min_dist = sit_i->first + sit_j->first - 2*sqrt(sit_i->first)*sqrt(sit_j->first);
	    
	    nchecks++;
	    
	    if (dist < (_thd*_thd)){
		double val =  exp(-dist/(0.5*_thd*_thd));
		
		wtmat[i].push_back(RowVal1(j, val));
		(degree[i]) += val;
		wtmat[j].push_back(RowVal1(i, val));
		(degree[j]) += val;
		
		nnz += 1;
	    }
	    if (min_dist>(_thd*_thd)){
	      break;
	    }
	}
    }
    printf("total nonzeros: %lu\n",nnz);

}
double WeightMatrix1::nnz_pct()
{
      
    double pct = 0.0;
    size_t nr = _degree.size();
    for(size_t ii=0; ii < nr ; ii++)
	pct += _wtmat[ii].size();
    
    printf("Wt Mat nnz: %u, total:%u\n",(unsigned int)pct, nr);
    pct /= (nr*nr);
    printf("Wt Mat nnz pct: %lf\n", pct);
    return pct;
}