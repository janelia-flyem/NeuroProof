cmake_minimum_required (VERSION 2.8) 

######################################################################
#
#      find Python interpreter
#
######################################################################

FIND_PACKAGE(PythonInterp 2 REQUIRED)

IF(PYTHONINTERP_FOUND)
    # check that Python version 2.x is used
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
                         "import sys; print(sys.version[0])"
                          OUTPUT_VARIABLE PYTHON_MAJOR_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF(NOT (${PYTHON_MAJOR_VERSION} EQUAL 2))
        MESSAGE(FATAL_ERROR "Python bindings require Python 2.x.")
    ENDIF()
ENDIF()

######################################################################
#
#      find Python library
#
######################################################################

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
                 "import sys; print sys.exec_prefix"
                  OUTPUT_VARIABLE PYTHON_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)

IF(APPLE AND ${PYTHON_PREFIX} MATCHES ".*framework.*")
    SET(PYTHON_LIBRARIES "${PYTHON_PREFIX}/Python"
        CACHE FILEPATH "Python libraries"
        FORCE)
ELSE()
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
                     "import sys; skip = 2 if sys.platform.startswith('win') else 1; print 'python' + sys.version[0:3:skip]"
                      OUTPUT_VARIABLE PYTHON_LIBRARY_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
    FIND_LIBRARY(PYTHON_LIBRARIES ${PYTHON_LIBRARY_NAME} HINTS "${PYTHON_PREFIX}" 
                 PATH_SUFFIXES lib lib64 libs DOC "Python libraries")
ENDIF()

######################################################################
#
#      find Python includes
#
######################################################################

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
                "from distutils.sysconfig import *; print get_python_inc()"
                OUTPUT_VARIABLE PYTHON_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
SET(PYTHON_INCLUDE_PATH ${PYTHON_INCLUDE}
    CACHE PATH "Path to Python include files"
    FORCE)

######################################################################
#
#      find default install directory for Python modules
#      (usually PYTHONDIR/Lib/site-packages)
#
######################################################################
IF(NOT DEFINED LIBDVID_PYTHON_INSTALL_DIR OR LIBDVID_PYTHON_INSTALL_DIR MATCHES "^$")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
                     "from distutils.sysconfig import *; print get_python_lib(1)"
                      OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
    FILE(TO_CMAKE_PATH ${PYTHON_SITE_PACKAGES} LIBDVID_PYTHON_INSTALL_DIR)
ENDIF()
SET(LIBDVID_PYTHON_INSTALL_DIR ${LIBDVID_PYTHON_INSTALL_DIR}
    CACHE PATH "where to install the libdvid Python package" FORCE)
# this is the install path relative to CMAKE_INSTALL_PREFIX,
# use this in INSTALL() commands to get packaging right
FILE(RELATIVE_PATH LIBDVID_PYTHON_INSTALL_DIR ${CMAKE_INSTALL_PREFIX} ${LIBDVID_PYTHON_INSTALL_DIR})

######################################################################
#
#      find numpy include directory
#      (usually below PYTHONDIR/Lib/site-packages/numpy)
#
######################################################################
IF(NOT PYTHON_NUMPY_INCLUDE_DIR)
    # Note: we must suppress possible output of the 'from numpy... import *' command,
    #       because the output cannot be interpreted correctly otherwise
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
                     "import sys, os; sys.stdout = open(os.devnull, 'w'); from numpy.distutils.misc_util import *; sys.__stdout__.write(' '.join(get_numpy_include_dirs()))"
                      RESULT_VARIABLE PYTHON_NUMPY_NOT_FOUND
                      OUTPUT_VARIABLE PYTHON_NUMPY_INCLUDE_DIR
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF(NOT PYTHON_NUMPY_NOT_FOUND)
        FILE(TO_CMAKE_PATH ${PYTHON_NUMPY_INCLUDE_DIR} PYTHON_NUMPY_INCLUDE_DIR)
    ELSE()
        SET(PYTHON_NUMPY_INCLUDE_DIR "PYTHON_NUMPY_INCLUDE_DIR-NOTFOUND")
    ENDIF()
ENDIF()

SET(PYTHON_NUMPY_INCLUDE_DIR ${PYTHON_NUMPY_INCLUDE_DIR}
    CACHE PATH "Path to numpy include files" FORCE)

######################################################################
#
#      get numpy version
#
######################################################################
IF(NOT PYTHON_NUMPY_NOT_FOUND)
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
                            "import numpy; print(numpy.__version__)"
                    RESULT_VARIABLE _NUMPY_VERSION_FAIL
                    OUTPUT_VARIABLE NUMPY_VERSION
                    ERROR_VARIABLE _NUMPY_VERSION_ERROR_VALUE
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF(NOT _NUMPY_VERSION_FAIL)
        string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" _VER_CHECK "${NUMPY_VERSION}")
        if("${_VER_CHECK}" STREQUAL "")
            # The output from Python was unexpected. Raise an error always
            # here, because we found NumPy, but it appears to be corrupted somehow.
            message(FATAL_ERROR "Requested version from NumPy, got instead:\n${NUMPY_VERSION}\n")
        endif()
            # Get the major and minor version numbers
            string(REGEX REPLACE "\\." ";" _NUMPY_VERSION_LIST ${NUMPY_VERSION})
            list(GET _NUMPY_VERSION_LIST 0 NUMPY_VERSION_MAJOR)
            list(GET _NUMPY_VERSION_LIST 1 NUMPY_VERSION_MINOR)
            list(GET _NUMPY_VERSION_LIST 2 NUMPY_VERSION_PATCH)
    ELSE()
        message(FATAL_ERROR "Could not determine numpy version: ${_NUMPY_VERSION_ERROR_VALUE}")
    ENDIF()
ENDIF()

######################################################################
#
#      status output
#
######################################################################
IF(PYTHON_LIBRARIES AND PYTHON_INCLUDE_PATH)
    MESSAGE(STATUS "Found Python libraries: ${PYTHON_LIBRARIES}")
    MESSAGE(STATUS "Found Python includes:  ${PYTHON_INCLUDE_PATH}")
    SET(PYTHONLIBS_FOUND TRUE)
ELSE()
    MESSAGE(FATAL_ERROR "Could NOT find Python libraries and/or includes")
ENDIF()
    
IF(PYTHON_NUMPY_INCLUDE_DIR)
    MESSAGE(STATUS "Python numpy includes: ${PYTHON_NUMPY_INCLUDE_DIR}")
    MESSAGE(STATUS "Python numpy verion: ${NUMPY_VERSION}")
ELSE()
    MESSAGE(FATAL_ERROR "Could NOT find Python numpy ('import numpy.distutils.misc_util' failed)")
ENDIF()

set(LIBDVID_MIN_NUMPY_VERSION "1.7.0")
if (NUMPY_VERSION VERSION_LESS LIBDVID_MIN_NUMPY_VERSION)
    message(FATAL_ERROR "Numpy version (${NUMPY_VERSION}) is too old.  libdvid requires numpy >= ${LIBDVID_MIN_NUMPY_VERSION}")
endif()


######################################################################
#
#      configure package
#
######################################################################

include_directories(${PYTHON_INCLUDE_PATH} ${PYTHON_NUMPY_INCLUDE_DIR})

#add_library( _classifier_python SHARED src/Classifier.cpp ${CMAKE_SOURCE_DIR}/src/vigraRFclassifier.cpp ${CMAKE_SOURCE_DIR}/src/Classifier/opencvRFclassifier.cpp )

add_library( _classifier_python SHARED src/Classifier.cpp)
add_library( _agglomeration_python SHARED src/Agglomeration.cpp)
add_library( _focusedproofreading_python SHARED src/FocusedProofreading.cpp)

target_link_libraries( _classifier_python Classifier ${NEUROPROOF_EXT_LIBS} ${boostpython_LIB} ${PYTHON_LIBRARIES} )
set_target_properties( _classifier_python PROPERTIES PREFIX  ""
                                               DEBUG_POSTFIX "" )
target_link_libraries( _agglomeration_python ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS} ${boostpython_LIB} ${PYTHON_LIBRARIES} )
set_target_properties( _agglomeration_python PROPERTIES PREFIX  "")

target_link_libraries( _focusedproofreading_python ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS} ${boostpython_LIB} ${PYTHON_LIBRARIES} )
set_target_properties( _focusedproofreading_python PROPERTIES PREFIX  "")

if (APPLE)
    set_target_properties( _classifier_python PROPERTIES SUFFIX  ".so")
    set_target_properties( _agglomeration_python PROPERTIES SUFFIX  ".so")
    set_target_properties( _focusedproofreading_python PROPERTIES SUFFIX  ".so")
endif()

if (NOT (APPLE OR MSVC))
    # FIXME: This may not be needed anymore because timing now uses std::chrono
    target_link_libraries( _classifier_python rt)
    target_link_libraries( _agglomeration_python rt)
    target_link_libraries( _focusedproofreading_python rt)
endif()

install(TARGETS _classifier_python
    RUNTIME DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/Classifier
    LIBRARY DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/Classifier)
install(TARGETS _agglomeration_python
    RUNTIME DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/Agglomeration
    LIBRARY DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/Agglomeration)
install(TARGETS _focusedproofreading_python
    RUNTIME DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/FocusedProofreading
    LIBRARY DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/FocusedProofreading)

install(FILES neuroproof/__init__.py
        DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof)
install(FILES neuroproof/Classifier/__init__.py
    DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/Classifier)
install(FILES neuroproof/Agglomeration/__init__.py
    DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/Agglomeration)
install(FILES neuroproof/FocusedProofreading/__init__.py
    DESTINATION ${LIBDVID_PYTHON_INSTALL_DIR}/neuroproof/FocusedProofreading)

enable_testing() 

add_test("test_agglom_python"
    ${PYTHON_EXE}
    ${CMAKE_SOURCE_DIR}/integration_tests/testpyagglom.py
    ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_labels.h5 
    ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_prediction.h5
    ${CMAKE_SOURCE_DIR}/integration_tests/inputs/250-1_agglo_itr1_trial1_opencv_rf_tr255.xml
    0.2
)

