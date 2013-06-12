#ifndef VOLUMEDATA_H
#define VOLUMEDATA_H

#include <vigra/multi_array.hxx>
#include <vigra/hdf5impex.hxx>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace NeuroProof {

template <typename T>
class VolumeData;

typedef VolumeData<double> VolumeProb;
typedef boost::shared_ptr<VolumeProb> VolumeProbPtr;
typedef boost::shared_ptr<VolumeData<char> > VolumeGrayPtr;

// This class might be an unnecessary shell over multiarray.  In theory,
// common functionality (perhaps in visualization) between data volumes
// could be shared using this as the base class.  There will be derived
// classes from this which will be implementing new functionality compared
// to MultiArray
template <typename T>
class VolumeData : public vigra::MultiArray<3, T> {
  public:
    static boost::shared_ptr<VolumeData<T> > create_volume();
    static boost::shared_ptr<VolumeData<T> > create_volume(
            const char * h5_name, const char * dset);
    static std::vector<boost::shared_ptr<VolumeData<T> > >
        create_volume_array(const char * h5_name, const char * dset);

    void serialize(const char* h5_name, const char * h5_path); 

    // TODO: provide some visualization functionality

  protected:
    VolumeData() : vigra::MultiArray<3,T>() {}
    VolumeData(const vigra::MultiArrayView<3, T>& view_) : vigra::MultiArray<3,T>(view_) {}
};


template <typename T>
boost::shared_ptr<VolumeData<T> > VolumeData<T>::create_volume()
{
    return boost::shared_ptr<VolumeData<T> >(new VolumeData<T>); 
}

template <typename T>
boost::shared_ptr<VolumeData<T> > VolumeData<T>::create_volume(
        const char * h5_name, const char * dset)

{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 3, "Dataset must be 3-dimensional.");

    VolumeData<T>* volumedata = new VolumeData<T>;
    vigra::TinyVector<long long unsigned int,3> shape(info.shape().begin());
    volumedata->reshape(shape);
    vigra::readHDF5(info, *volumedata);

    return boost::shared_ptr<VolumeData<T> >(volumedata); 
}

#include <iostream>
// assume last dimension gives channel
template <typename T>
std::vector<boost::shared_ptr<VolumeData<T> > > 
    VolumeData<T>::create_volume_array(const char * h5_name, const char * dset)
{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 4, "Dataset must be 4-dimensional.");

    vigra::TinyVector<long long unsigned int,4> shape(info.shape().begin());
    vigra::MultiArray<4, T> volumedata_temp(shape);
    vigra::readHDF5(info, volumedata_temp);
    //vigra::MultiArrayView<4,T,vigra::StridedArrayTag> volumedata_view = 
    //    volumedata_temp.transpose();
    volumedata_temp = volumedata_temp.transpose();

    std::vector<VolumeProbPtr> vol_array;
    vigra::TinyVector<long long unsigned int,3> shape2;
    shape2[0] = shape[3];
    shape2[1] = shape[2];
    shape2[2] = shape[1];

    //unsigned long long offset = 0;
    for (int i = 0; i < shape[0]; ++i) {
        VolumeData<T>* volumedata = new VolumeData<T>;
        vigra::TinyVector<vigra::MultiArrayIndex, 1> channel(i);
        (*volumedata) = volumedata_temp.bindOuter(channel); 
        
        vol_array.push_back(boost::shared_ptr<VolumeData<T> >(volumedata));
    }

    return vol_array; 
}

template <typename T>
void VolumeData<T>::serialize(const char* h5_name, const char * h5_path)
{
    //vigra::MultiArrayView<3, T> volume_view = this->transpose();
    vigra::writeHDF5(h5_name,  h5_path, *this);
}


#define volume_forXYZ(volume,x,y,z) \
    for (int z = 0; z < (int)(volume).shape(2); ++z) \
        for (int y = 0; y < (int)(volume).shape(1); ++y) \
            for (int x = 0; x < (int)(volume).shape(0); ++x) 


}

#endif
