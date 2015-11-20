#include <Python.h>
#include <boost/python.hpp>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#importing-the-api
#define PY_ARRAY_UNIQUE_SYMBOL libdvid_PYTHON_BINDINGS
#include <numpy/arrayobject.h>

#include "converters.hpp"

using namespace boost::python;
using std::vector;

namespace NeuroProof { namespace python {

VolumeLabelPtr agglomerate(VolumeLabelPtr labels, vector<VolumeProbPtr> prob_array)
{
    return labels;
}

BOOST_PYTHON_MODULE(_agglomeration_python)
{
    // http://docs.scipy.org/doc/numpy/reference/c-api.array.html#importing-the-api
    import_array();
    ndarray_to_segmentation();
    ndarray_to_predictionarray(); 
 
    def("agglomerate" , agglomerate);
}


}}
