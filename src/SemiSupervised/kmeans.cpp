#include "kmeans.h"

using namespace NeuroProof;


void kMeans::compute_centers(std::vector< std::vector<double> >& data, std::vector<unsigned int>& cidx)
{

// //     FILE *fp = fopen("semi-supervised/tmp_features_kmeans.txt","wt");
// //     for(size_t ii=0; ii< data.size(); ii++){
// // 	for (size_t jj=0; jj< data[ii].size(); jj++)
// // 	    fprintf(fp, "%lf ",data[ii][jj]);
// // 	fprintf(fp,"\n");
// //     }
// //     fclose(fp);
  
    _ndata = data.size();
    _dim = data[0].size();
  
    std::vector< std::vector<double> > ctrs;
    initial_centers(data, ctrs);
    
    unsigned int iter=0; 
    double delta_dist0 = DBL_MAX, delta_dist, delta_diff=DBL_MAX;
    
    while (iter < _maxIter && delta_diff > _minDelta){
	  delta_dist = kmiter(data, ctrs, cidx);
	  iter++;
	  delta_diff = delta_dist0 - delta_dist;
	  delta_dist0 = delta_dist;
    }
    if(delta_diff<= _minDelta)
      printf("Converged upto the tolerance %.4lf\n",_minDelta);
    if(iter>=_maxIter)
      printf("Maximum number of iter %d reached\n", _maxIter);
    
// //     fp = fopen("semi-supervised/tmp_ctrs_kmeans.txt","wt");
// //     for(size_t ii=0; ii< _K; ii++){
// // 	for (size_t jj=0; jj< ctrs[ii].size(); jj++)
// // 	    fprintf(fp, "%lf ", ctrs[ii][jj]);
// // 	fprintf(fp,"\n");
// //     }
// //     fclose(fp);
    
    
}
void kMeans::initial_centers(std::vector< std::vector<double> >& data, std::vector< std::vector<double> >& ctrs)
{
    

    std::vector<unsigned int> sidx;
    for(size_t ii = 0; ii < _ndata; ii++)
	sidx.push_back(ii);
    random_shuffle(sidx.begin(), sidx.end());
    
    sidx.erase(sidx.begin()+_K, sidx.end());
    ctrs.clear(); ctrs.resize(_K);
    for(size_t ii = 0; ii < _K; ii++){
	unsigned int idx = sidx[ii];
	ctrs[ii].insert(ctrs[ii].begin(), data[idx].begin(), data[idx].end()); 
    }
}


double kMeans::kmiter(std::vector< std::vector<double> >& data, std::vector< std::vector<double> >& ctrs,
	      std::vector<unsigned int>& idx_closest_pt)
{
    std::vector< std::vector<double> > ret_ctrs(_K);
    std::vector<double> count(_K, 0);
    
    std::vector<double> dist_closest_pt(_K, DBL_MAX);
    idx_closest_pt.clear(); idx_closest_pt.resize(_K, 0);
    
    double sqdist = 0;
    
    for(size_t ii=0; ii < _ndata; ii++ ){
	double mindist = DBL_MAX;
	unsigned int mind_idx = 0;
	for(size_t c=0; c < _K; c++){
	    double dist = 0;
	    for(size_t jj=0; jj < _dim ; jj++){
		dist += (data[ii][jj] - ctrs[c][jj])*(data[ii][jj] - ctrs[c][jj]);
	    }
	    if (dist < mindist){
		mindist = dist;
		mind_idx = c;
	    }
	}
	
	sqdist += mindist;
	
	if (!(ret_ctrs[mind_idx].size() > 0))
	    ret_ctrs[mind_idx].resize(_dim, 0);
	for(size_t jj=0; jj < _dim ; jj++)
	    ret_ctrs[mind_idx][jj] += data[ii][jj];
	
	(count[mind_idx])++;
	
	if (dist_closest_pt[mind_idx] > mindist){
	    dist_closest_pt[mind_idx] = mindist;
	    idx_closest_pt[mind_idx] = ii;
	}
    }
    
    for(size_t c =0; c < _K ; c++){
	if (count[c]){
	  for(size_t jj=0; jj < _dim ; jj++){
	      ret_ctrs[c][jj] /= count[c];
	      ctrs[c][jj] = ret_ctrs[c][jj];
	  }
	}
    }
    
    return sqdist;
  
}
