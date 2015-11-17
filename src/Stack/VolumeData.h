/*!
 * Defines a class that inherits from Vigra multiarray. It assumes
 * that all data volumes will have 3 dimensions.  If a 2D or 1D
 * volume is needed, the volume shape should be X,Y,1 or X,1,1
 * respectively.  Please examine documentation in Vigra 
 * (http://hci.iwr.uni-heidelberg.de/vigra) for more information on
 * how to use multiarray and the algorithms that are available for
 * this data type.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef VOLUMEDATA_H
#define VOLUMEDATA_H

#include <vigra/multi_array.hxx>

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <Utilities/ErrMsg.h>
#include <Utilities/Glb.h>

namespace NeuroProof {

// forward declaration
template <typename T>
class VolumeData;

// defines some of the common volume types used in NeuroProof
typedef float Prob_t;
typedef VolumeData<Prob_t> VolumeProb;
typedef VolumeData<uint8> VolumeGray;
typedef boost::shared_ptr<VolumeProb> VolumeProbPtr;
typedef boost::shared_ptr<VolumeGray> VolumeGrayPtr;

/*!
 * This class defines a 3D volume of any type.  In particular,
 * it inherits properties of multiarray and provides functionality
 * for creating new volumes.  Importing file data should use
 * functionality in the IO library.  This interface stipulates
 * that new VolumeData objects get created on the heap and are
 * encapsulated in shared pointers.
*/
template <typename T>
class VolumeData : public vigra::MultiArray<3, T> {
  public:
    /*!
     * Static function to create an empty volume data object.
     * \return shared pointer to volume data
    */
    static boost::shared_ptr<VolumeData<T> > create_volume();
    
    /*!
     * Copy constructor to create VolumeData from a multiarray view.  It
     * just needs to call the multiarray constructor with the view.
     * \param view_ view to a multiarray
    */
    VolumeData(const vigra::MultiArrayView<3, T>& view_) : vigra::MultiArray<3,T>(view_) {}

  protected:
    /*!
     * Private definition of constructor to prevent stack allocation.
    */
    VolumeData() : vigra::MultiArray<3,T>() {}
    
};


template <typename T>
boost::shared_ptr<VolumeData<T> > VolumeData<T>::create_volume()
{
    return boost::shared_ptr<VolumeData<T> >(new VolumeData<T>); 
}

// convenience macro for iterating a multiarray and derived classes
#define volume_forXYZ(volume,x,y,z) \
    for (int z = 0; z < (int)(volume).shape(2); ++z) \
        for (int y = 0; y < (int)(volume).shape(1); ++y) \
            for (int x = 0; x < (int)(volume).shape(0); ++x) 


}

#endif
