#include <Python.h>
#include <boost/python.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/slice.hpp>
#include <vector>

// We intentionally omit numpy/arrayobject.h here because cpp files need to be careful with exactly when this is imported.
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#importing-the-api
// Therefore, we assume the cpp file that included converters.hpp has already included numpy/arrayobject.h
// #include <numpy/arrayobject.h>

#include <Stack/Stack.h>

namespace NeuroProof { namespace python {


/*!
 * Converts between 3D numpy ndarray,  C++ 3D arrays objects.
 * The data is currently copied back and forth.  The ndarray
 * is spec'd to be numpy.uint32.
*/
struct ndarray_to_segmentation
{
    ndarray_to_segmentation()
    {
        using namespace boost::python;
        // Register python -> C++
        converter::registry::push_back( &convertible, &construct, type_id<VolumeLabelPtr>() );

        // Register C++ -> Python
        to_python_converter<VolumeLabelPtr, ndarray_to_segmentation >();
}

    //! Check if the given object is an ndarray
    static void* convertible(PyObject* obj_ptr)
    {
        using namespace boost::python;
        if (!PyArray_Check(obj_ptr))
        {
            return 0;
        }
        // We could also check shape, dtype, etc. here, but it's more helpful to
        // wait until construction time, and then throw a specific exception to
        // tell the user what he did wrong.
        return obj_ptr;
    }

    //! Converts the given numpy ndarray object into a VolumeLabelPtr object.
    //! NOTE: The data from the ndarray is *copied* into the new VolumeLabelPtr object.
    static void construct( PyObject* obj_ptr, 
            boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        using namespace boost::python;
        assert(PyArray_Check(obj_ptr));
        object ndarray = object(handle<>(borrowed(obj_ptr)));

        // Verify ndarray.dtype
        std::string dtype = extract<std::string>(str(ndarray.attr("dtype")));
        if (dtype != "uint32")
        {
            std::ostringstream ssMsg;
            ssMsg << "Volume has wrong dtype.  Expected " << "uint32" << ", got " << dtype;
            throw ErrMsg(ssMsg.str());
        }

        // Verify ndarray dimensionality.
        int ndarray_ndim = extract<int>(ndarray.attr("ndim"));
        if (ndarray_ndim != 3)
        {
            std::string shape = extract<std::string>(str(ndarray.attr("shape")));
            std::ostringstream ssMsg;
            ssMsg << "Volume is not exactly 3D.  Shape is " << shape;
            throw ErrMsg( ssMsg.str() );
        }

        // Verify ndarray memory order
        if (!ndarray.attr("flags")["C_CONTIGUOUS"])
        {
            throw ErrMsg("Volume is not C_CONTIGUOUS");
        }

        // Extract dims from ndarray.shape (which is already in C-order)
        object shape = ndarray.attr("shape");
        typedef stl_input_iterator<unsigned int> shape_iter_t;
        std::vector<unsigned int> dims;
        dims.assign( shape_iter_t(shape), shape_iter_t() );

        // Obtain a pointer to the array's data
        PyArrayObject * array_object = reinterpret_cast<PyArrayObject *>( ndarray.ptr() );
        Label_t const * voxel_data = static_cast<Label_t const *>( PyArray_DATA(array_object) );

        // Grab pointer to memory into which to construct the DVIDVoxels
        void* storage = ((converter::rvalue_from_python_storage<VolumeLabelPtr>*) data)->storage.bytes;
        // point VolumeLabelData to boost.python chunk pointer
        
        // Create smart pointer from ndarray data using "in-place" new().
        // (memory reclaimed)
        VolumeLabelPtr * volumeptr = new (storage) VolumeLabelPtr; 

        *volumeptr = VolumeLabelData::create_volume();
        // coordinate system in vigra in fortran order
        (*volumeptr)->reshape(vigra::MultiArrayShape<3>::type(dims[2], dims[1], dims[0]));

        // do not reuse memory since data will be modified 
        volume_forXYZ(**volumeptr,x,y,z) {
            (*volumeptr)->set(x,y,z, *voxel_data);
            voxel_data++;
        }

        // Stash the memory chunk pointer for later use by boost.python
        data->convertible = storage;
    }

    //! Converts (copies) the given VolumeLabelPtr object into a numpy array
    static PyObject* convert( VolumeLabelPtr volume )
    {
        using namespace boost::python;
        std::vector<unsigned int> dims;
        dims.push_back(volume->shape(0));
        dims.push_back(volume->shape(1));
        dims.push_back(volume->shape(2));

        // REVERSE from Fortran order (XYZ) to C-order (ZYX)
        // (PyArray_SimpleNewFromData will create in C-order.)
        std::vector<npy_intp> numpy_dims( dims.rbegin(),
                                          dims.rend() );

        // create new array managed by the python object
        PyObject * array_object = PyArray_SimpleNew( numpy_dims.size(),
                                                             &numpy_dims[0],
                                                             NPY_UINT32);
        if (!array_object)
        {
            throw ErrMsg("Failed to create array!");
        }
        
        PyArrayObject * ndarray = reinterpret_cast<PyArrayObject *>( array_object );

        Label_t * voxel_data = static_cast<Label_t*>( PyArray_DATA(ndarray) );
        
        // copy data to volume
        volume_forXYZ(*volume,x,y,z) {
            *voxel_data = (*volume)(x,y,z);
            voxel_data++;
        }       

        // return array object
        return array_object;
    }
};

/*!
 * Converts from a 4D numpy array consisting several channels
 * of 3D volume predictions to a C++ vector of vigra arrays.
 * The ndarray is spec'd to be numpy.float32.
*/
struct ndarray_to_predictionarray 
{
    ndarray_to_predictionarray()
    {
        using namespace boost::python;
        // Register python -> C++
        converter::registry::push_back( &convertible, &construct, type_id<std::vector<VolumeProbPtr> >() );
    }

    //! Check if the given object is an ndarray
    static void* convertible(PyObject* obj_ptr)
    {
        using namespace boost::python;
        if (!PyArray_Check(obj_ptr))
        {
            return 0;
        }
        // We could also check shape, dtype, etc. here, but it's more helpful to
        // wait until construction time, and then throw a specific exception to
        // tell the user what he did wrong.
        return obj_ptr;
    }

    //! Converts the given numpy ndarray object into a vector<VolumeProbPtr>.
    //! NOTE: The data from the ndarray is *copied* into the new VolumePropPtr object.
    static void construct( PyObject* obj_ptr, 
            boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        using namespace boost::python;
        assert(PyArray_Check(obj_ptr));
        object ndarray = object(handle<>(borrowed(obj_ptr)));

        // Verify ndarray.dtype
        std::string dtype = extract<std::string>(str(ndarray.attr("dtype")));
        if (dtype != "float32") {
            std::ostringstream ssMsg;
            ssMsg << "Volume has wrong dtype.  Expected " << "float32" << ", got " << dtype;
            throw ErrMsg(ssMsg.str());
        }

        // Verify ndarray dimensionality.
        int ndarray_ndim = extract<int>(ndarray.attr("ndim"));
        if (ndarray_ndim != 4) {
            std::string shape = extract<std::string>(str(ndarray.attr("shape")));
            std::ostringstream ssMsg;
            ssMsg << "Volume is not exactly 4D.  Shape is " << shape;
            throw ErrMsg( ssMsg.str() );
        }

        // Verify ndarray memory order
        if (!ndarray.attr("flags")["C_CONTIGUOUS"]) {
            throw ErrMsg("Volume is not C_CONTIGUOUS");
        }

        // Extract dims from ndarray.shape (which is in C-order)
        object shape = ndarray.attr("shape");
        typedef stl_input_iterator<unsigned int> shape_iter_t;
        std::vector<unsigned int> dims;
        dims.assign( shape_iter_t(shape), shape_iter_t() );

        // Obtain a pointer to the array's data
        PyArrayObject * array_object = reinterpret_cast<PyArrayObject *>( ndarray.ptr() );
        Prob_t const * prob_data = static_cast<Prob_t const *>( PyArray_DATA(array_object) );

        // Grab pointer to memory into which to construct the DVIDVoxels
        void* storage = ((converter::rvalue_from_python_storage<VolumeLabelPtr>*) data)->storage.bytes;
        // point VolumeLabelData to boost.python chunk pointer
        
        // Create smart pointer from ndarray data using "in-place" new().
        std::vector<VolumeProbPtr> * probarray = new (storage) std::vector<VolumeProbPtr>; 

        for (int i = 0; i < dims[3]; ++i) {
            boost::shared_ptr<VolumeProb> probdata = VolumeProb::create_volume();

            probdata->reshape(vigra::MultiArrayShape<3>::type(dims[2], dims[1], dims[0]));
            (*probarray).push_back(probdata);

        }

        // could potentially reuse this memory if I change the interface ??
        volume_forXYZ(*((*probarray)[0]),x,y,z) {
            for (int i = 0; i < dims[3]; ++i) {
                (*(*probarray)[i])(x,y,z) = *prob_data;
                prob_data++;
            }
        }

        // Stash the memory chunk pointer for later use by boost.python
        data->convertible = storage;
    }
};

}} // namespace NeuroProof::python
