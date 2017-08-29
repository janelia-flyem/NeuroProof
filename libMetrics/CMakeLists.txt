cmake_minimum_required (VERSION 2.8)
project (Metrics)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")

FIND_PACKAGE(PythonInterp)
FIND_PACKAGE(PythonLibs)
FIND_PACKAGE(Boost REQUIRED COMPONENTS system)

######################################################################
#
#  Find boost::python library
#
#  (Copied from vigra/config/FindVIGRANUMPY_DEPENDENCIES.cmake)
#
# 'FIND_PACKAGE(Boost COMPONENTS python)' is unreliable because it often selects
# boost_python for the wrong Python version
#
######################################################################
IF(Boost_FOUND)

    IF(Boost_USE_MULTITHREADED)
        # define names for thread-safe library variants
        SET(BOOST_PYTHON_NAMES
                boost_python-py${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}-mt
                boost_python-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}-mt
                boost_python${PYTHON_VERSION_MAJOR}-mt
                boost_python-mt)
    ENDIF()

    IF(Boost_LIB_SUFFIX)
        SET(BOOST_PYTHON_NAMES ${BOOST_PYTHON_NAMES}
            # Windows with mangled library names
            boost_python${PYTHON_VERSION_MAJOR}${Boost_LIB_SUFFIX}
            boost_python${Boost_LIB_SUFFIX})
    ENDIF()

    # define names for boost_python library variants
    # (may or may not be thread-safe)
    SET(BOOST_PYTHON_NAMES ${BOOST_PYTHON_NAMES}
            # Linux with multiple Python versions
            boost_python-py${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}
            # Gentoo
            boost_python-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}
            # Mac with Python 3
            boost_python${PYTHON_VERSION_MAJOR}
            # default
            boost_python)

    FIND_LIBRARY(Boost_PYTHON_LIBRARY
                 NAMES ${BOOST_PYTHON_NAMES}
                 NAMES_PER_DIR
                 HINTS "${Boost_LIBRARY_DIR}"
                 DOC "boost_python libraries")
ENDIF()

if(Boost_PYTHON_LIBRARY)
    MESSAGE(STATUS "Found boost_python library: ${Boost_PYTHON_LIBRARY}")
else()
    MESSAGE(FATAL_ERROR "Could NOT find boost_python library")
endif()


# create library
add_library (NeuroProofMetrics SHARED pythonMetrics.cpp) 

set (PYLIBLOC ${NEUROPROOF_PYTHON_INSTALL_DIR})
set (PYTHON_LIBRARY_FILE ${PYTHON_LIBRARIES})

set (hdf5_LIBRARIES hdf5 hdf5_hl)
set (vigra_LIB vigraimpex)
set (opencv_LIBS opencv_ml opencv_core)
set (boost_LIBS ${Boost_SYSTEM_LIBRARY_RELEASE} ${Boost_PYTHON_LIBRARY})

include_directories (BEFORE ${PYTHON_INCLUDE_PATH})

# include other external libraries
set (METRICS_EXT_LIBS  ${boost_LIBS} ${PYTHON_LIBRARY_FILE})

#SET_TARGET_PROPERTIES (NeuroProofMetrics
#  PROPERTIES BUILD_WITH_INSTALL_RPATH 1
#  INSTALL_NAME_DIR ${PYLIBLOC}
#)

target_link_libraries (NeuroProofMetrics Stack Rag FeatureManager Algorithms ${METRICS_EXT_LIBS} )
    
# copy neuroproof metric lib to the proper location

if (APPLE)
    set_target_properties( NeuroProofMetrics PROPERTIES SUFFIX  ".so")
endif()

install(TARGETS NeuroProofMetrics
        DESTINATION ${PYLIBLOC})

