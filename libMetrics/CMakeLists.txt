cmake_minimum_required (VERSION 2.8)
project (Metrics)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")


# create library
add_library (NeuroProofMetrics SHARED pythonMetrics.cpp) 

if (NOT ${BUILDEM_DIR} STREQUAL "None")
    include (python)
   
    # set buildem dependencies
    set (PYLIBLOC ${BUILDEM_DIR}/lib/python2.7/site-packages/)
    set (METRICS_DEPS ${boost_NAME} ${python_NAME})
    add_dependencies (NeuroProofMetrics ${METRICS_DEPS})
else ()
   
    set (PYLIBLOC ${NEUROPROOF_PYTHON_INSTALL_DIR})
    set (PYTHON_LIBRARY_FILE ${PYTHON_LIBRARIES})

    set (hdf5_LIBRARIES hdf5 hdf5_hl)
    set (vigra_LIB vigraimpex)
    set (opencv_LIBS opencv_ml opencv_core)
    set (boost_LIBS boost_system boost_python)
endif()

include_directories (BEFORE ${PYTHON_INCLUDE_PATH})

# include other external libraries
set (METRICS_EXT_LIBS  ${boost_LIBS} ${PYTHON_LIBRARY_FILE})

target_link_libraries (NeuroProofMetrics Stack Rag FeatureManager Algorithms ${METRICS_EXT_LIBS} )
    
# copy neuroproof metric lib to the proper location
get_target_property (NeuroProofMetrics_lib NeuroProofMetrics LOCATION)

install(FILES ${NeuroProofMetrics_lib} 
        DESTINATION ${PYLIBLOC})

