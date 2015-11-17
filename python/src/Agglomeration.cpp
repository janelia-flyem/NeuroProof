#include <Python.h>
#include <boost/python.hpp>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#importing-the-api
#define PY_ARRAY_UNIQUE_SYMBOL libdvid_PYTHON_BINDINGS
#include <numpy/arrayobject.h>

#include "converters.hpp"

using namespace boost::python;

namespace NeuroProof { namespace python {

VolumeLabelPtr agglomerate(VolumeLabelPtr labels)
{
    std::cout << "inside: " << (*labels)(0,0,0) << std::endl;
    std::cout << "shape: " << labels->shape(0) << " " << labels->shape(1) << " " << labels->shape(2) << std::endl;
    return labels;
}

BOOST_PYTHON_MODULE(_agglomeration_python)
{
    // http://docs.scipy.org/doc/numpy/reference/c-api.array.html#importing-the-api
    import_array();
    ndarray_to_segmentation();
 
    def("agglomerate" , agglomerate);
}


}}
