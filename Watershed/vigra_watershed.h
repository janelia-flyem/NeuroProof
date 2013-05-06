#ifndef _vigra_watershed
#define _vigra_watershed


#include <vigra/seededregiongrowing3d.hxx>
#include <vigra/multi_array.hxx>

using namespace vigra;
 

typedef vigra::MultiArray<3,unsigned int> LblVolume;
typedef vigra::MultiArray<3,double> DVolume;


class VigraWatershed{
    size_t _depth; 	
    size_t _width; 	
    size_t _height; 	

public:
    VigraWatershed(size_t depth, size_t height, size_t width): _depth(depth), _height(height), _width(width) {};  	

    void run_watershed(double* data_vol, unsigned int* lbl_vol){ 	
    	DVolume src(DVolume::difference_type(_depth, _height, _width));
	LblVolume dest(LblVolume::difference_type(_depth, _height, _width));

    	unsigned long volsz = _depth*_height*_width;

    	unsigned long nnz=0; 	
    	
/*
	// *C* for debug only
	for(unsigned long i=0; i< volsz; i++){
	    unsigned int lbl = lbl_vol[i];
	    if (lbl==0){
	    	nnz++;	
	    }
    	} 
    	printf("total #zeros 1D: %lu\n",nnz);
*/

	unsigned long plane_size = _height*_width;
	nnz = 0;
	for(unsigned int d=0; d < _depth; d++){
	    unsigned long z1d = d*plane_size;
	    for(unsigned int i=0; i < _height; i++){
		unsigned long x1d = i*_width;
		for(unsigned int j=0; j < _width; j++){
		    unsigned long l1d = z1d+ x1d + j;
		    src(d,i,j) = data_vol[l1d];
		    dest(d,i,j) = lbl_vol[l1d];			
		    if (lbl_vol[l1d] == 0){
		    //if (dest(d,i,j) == 0){
			int pp=1;
			nnz++;
		    }
		}	
	    }	 	
	}
//    	printf("total #zeros in vigra: %lu\n",nnz);

    	vigra::ArrayOfRegionStatistics<vigra::SeedRgDirectValueFunctor<double> >
                                              stats;

    	
    	vigra:: seededRegionGrowing3D(srcMultiArrayRange(src), destMultiArray(dest),
                               destMultiArray(dest), stats);


	unsigned long nnz2=0;
	for(size_t d=0; d < _depth; d++){
	    unsigned long z1d = d*plane_size;
	    for(size_t i=0; i < _height; i++){
		unsigned long x1d = i*_width;
		for(size_t j=0; j < _width; j++){
		    unsigned long l1d = z1d+ x1d + j;
		    lbl_vol[l1d] = dest(d,i,j);			
		    if (lbl_vol[l1d] == 0){
			int pp=1;
			nnz2++;
		    }
		}	
	    }	 	
	}
    	printf("total #zeros in vol before and after wshed: %lu,  %lu\n",nnz, nnz2);
    }	
};

#endif
