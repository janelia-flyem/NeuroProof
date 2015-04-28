cmake_minimum_required (VERSION 2.8)
project (Metrics)

# create library
add_library (NeuroProofMetrics SHARED pythonMetrics.cpp) 

if (NOT ${BUILDEM_DIR} STREQUAL "None")
    # disable vigra numpy build
    SET(DISABLE_VIGRANUMPY "TRUE")

    # set binary location
    set (BUILDLOC ${BUILDEM_DIR})
    
    include (hdf5)
    include (python)
    include (boost)
    include (jsoncpp)
    include (vigra)
    include (opencv)

    # set buildem dependencies
    set (PYLIBLOC ${BUILDEM_DIR}/lib/python2.7/site-packages)
    set (METRICS_DEPS ${boost_NAME} ${hdf5_NAME} ${vigra_NAME} ${opencv_NAME} ${python_NAME} ${jsoncpp_NAME} )
    add_dependencies (NeuroProofMetrics ${METRICS_DEPS})
else ()
    # set binary location
    set (BUILDLOC ${CMAKE_SOURCE_DIR})
    
    # initial packages if not using buildem 
    FIND_PACKAGE(PythonLibs)
    FIND_PACKAGE(Boost)
    
    set (PYLIBLOC ${CMAKE_SOURCE_DIR}/lib)
    set (PYTHON_LIBRARY_FILE ${PYTHON_LIBRARIES})

    set (hdf5_LIBRARIES hdf5 hdf5_hl)
    set (vigra_LIB vigraimpex)
    set (opencv_LIBS opencv_ml opencv_core)
    set (boost_LIBS boost_thread boost_system boost_program_options boost_python boost_unit_test_framework boost_filesystem)
endif()

include_directories (BEFORE ${PYTHON_INCLUDE_PATH})

# add stack and rag directory dependency
add_subdirectory (../Stack "${CMAKE_CURRENT_BINARY_DIR}/Stack")
add_subdirectory (../Rag "${CMAKE_CURRENT_BINARY_DIR}/Rag")
add_subdirectory (../FeatureManager "${CMAKE_CURRENT_BINARY_DIR}/FeatureManager")


# include other external libraries
set (METRICS_EXT_LIBS ${json_LIB} ${vigra_LIB} ${opencv_LIBS} ${hdf5_LIBRARIES} ${boost_LIBS} ${PYTHON_LIBRARY_FILE})

target_link_libraries (NeuroProofMetrics Stack Rag FeatureManager ${METRICS_EXT_LIBS} )
    
# copy neuroproof metric lib to the proper location
get_target_property (NeuroProofMetrics_lib NeuroProofMetrics LOCATION)
add_custom_command (
    TARGET NeuroProofMetrics
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofMetrics_lib} ${PYLIBLOC})
