#define NEUROPROOF_INIT_NUMPY_CXX
#include "init_numpy.h"

// import_array() is a macro whose return type changed in python 3
// https://mail.scipy.org/pipermail/numpy-discussion/2010-December/054350.html
#if PY_MAJOR_VERSION >= 3

    int init_numpy()
    {
        import_array();
    }

#else

    void init_numpy()
    {
        import_array();
    }

#endif
