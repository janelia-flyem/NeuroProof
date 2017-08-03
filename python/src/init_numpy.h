#ifndef NEUROPROOF_INIT_NUMPY_HXX
#define NEUROPROOF_INIT_NUMPY_HXX

#include <Python.h>

#ifdef Py_ARRAYOBJECT_H
  #error "You must include init_numpy.h BEFORE including <numpy/arrayobject.h>"
#endif

#define PY_ARRAY_UNIQUE_SYMBOL neuroproof_PYTHON_BINDINGS
#ifndef NEUROPROOF_INIT_NUMPY_CXX
  #define NO_IMPORT_ARRAY
#endif

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#importing-the-api
#include <numpy/arrayobject.h>

#if PY_MAJOR_VERSION >= 3
    int init_numpy();
#else
    void init_numpy();
#endif

#endif // NEUROPROOF_INIT_NUMPY_HXX
