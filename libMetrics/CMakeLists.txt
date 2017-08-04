cmake_minimum_required (VERSION 2.8)
project (Metrics)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")


# create library
add_library (NeuroProofMetrics SHARED pythonMetrics.cpp) 

set (PYLIBLOC ${NEUROPROOF_PYTHON_INSTALL_DIR})
set (PYTHON_LIBRARY_FILE ${PYTHON_LIBRARIES})

set (hdf5_LIBRARIES hdf5 hdf5_hl)
set (vigra_LIB vigraimpex)
set (opencv_LIBS opencv_ml opencv_core)
set (boost_LIBS boost_system boost_python)

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

