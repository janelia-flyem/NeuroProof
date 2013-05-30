CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project (NeuroProof)

include (ExternalProject)

set (RUN_ENVIRONMENT "Workstation" CACHE TYPE STRING)
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif ()

set (CMAKE_CXX_FLAGS_RELEASE "-O3")
set (CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set (CMAKE_CXX_LINK_FLAGS "-O3")
set (CMAKE_DEBUG_POSTFIX "-g")


################################################################################
# Check if BUILDEM_DIR has already been assigned.  If not, create a default.
set (BUILDEM_DIR "None" CACHE TYPE STRING)

if (${BUILDEM_DIR} STREQUAL "None")
    message (FATAL_ERROR "ERROR: Buildem directory (for all downloads & builds) should be specified via -DBUILDEM_DIR=<path> on cmake command line.")
endif ()

message ("FlyEM downloads and builds will be placed here: ${BUILDEM_DIR}")

###############################################################################

###############################################################################
# Download and install buildem, if it isn't already in BUILDEM_DIR.
set (BUILDEM_REPO_DIR ${BUILDEM_DIR}/src/buildem)
if (NOT EXISTS ${BUILDEM_REPO_DIR}/python.cmake)
    message ("Installing buildem repo...")
    ExternalProject_Add(buildem
        PREFIX ${BUILDEM_DIR}
        GIT_REPOSITORY http://github.com/janelia-flyem/buildem.git
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        CONFIGURE_COMMAND "" 
        BUILD_COMMAND ""
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND ""
    )
    message ("\n**********************************************************\n")
    message ("\nAfter running make, you must re-run the cmake command once")
    message ("buildem has been downloaded!\n")
    message ("\n***********************************************************\n")
else ()
    ###############################################################################

    # Use modules from the downloaded buildem
    set (CMAKE_MODULE_PATH ${BUILDEM_REPO_DIR})
    message("Using cmake modules from ${BUILDEM_REPO_DIR}")

    # Download and compile dependencies
    include (python)
    include (jsoncpp)
    include (boost)
    include (vigra)
    include (hdf5)
    include (opencv)
    
    # Compile NeuroProof components

    add_subdirectory (Rag)
    add_subdirectory (FeatureManager)
    add_subdirectory (Algorithms)
    add_subdirectory (Classifier)
    add_subdirectory (DataStructures)

    set (NEUROPROOF_INT_LIBS Rag FeatureManager Algorithms Classifier DataStructures)
    set (NEUROPROOF_EXT_LIBS ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} ${boost_LIBS} ${PYTHON_LIBRARY_FILE})
    set (NEUROPROOF_DEPS ${jsoncpp_NAME} ${boost_NAME} ${hdf5_NAME} ${vigra_NAME} ${opencv_NAME})

    include_directories (BEFORE ${BUILDEM_DIR}/include ${PYTHON_INCLUDE_PATH})

    # Handle all of NeuroProof sources and dependent code
    add_executable (neuroproof_graph_analyze neuroproof_graph_analyze.cpp)
    add_executable (neuroproof_graph_analyze_gt neuroproof_graph_analyze_gt.cpp)
    add_executable (neuroproof_graph_learn neuroproof_graph_learn.cpp)
    add_executable (neuroproof_graph_predict neuroproof_graph_predict.cpp)

    target_link_libraries (neuroproof_graph_analyze ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    target_link_libraries (neuroproof_graph_analyze_gt ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    target_link_libraries (neuroproof_graph_learn ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    target_link_libraries (neuroproof_graph_predict ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    
    add_dependencies (neuroproof_graph_analyze ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_analyze_gt ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_learn ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_predict ${NEUROPROOF_DEPS})

    get_target_property (neuroproof_graph_analyze_exe neuroproof_graph_analyze LOCATION)
    get_target_property (neuroproof_graph_analyze_gt_exe neuroproof_graph_analyze_gt LOCATION)
    get_target_property (neuroproof_graph_learn_exe neuroproof_graph_learn LOCATION)
    get_target_property (neuroproof_graph_predict_exe neuroproof_graph_predict LOCATION)

    add_library (NeuroProofPriority SHARED pythonNeuroProofPriorityInterface.cpp)
    target_link_libraries (NeuroProofPriority ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    add_dependencies (NeuroProofPriority ${NEUROPROOF_DEPS})

    # need to build components separately to set python flag
    add_library (NeuroProofRag SHARED pythonRagInterface.cpp Rag/RagIO.cpp FeatureManager/FeatureManager.cpp FeatureManager/Features.cpp DataStructures/Stack.cpp Algorithms/MergePriorityFunction.cpp Algorithms/BatchMergeMRFh.cpp Refactoring/RagUtils2.cpp)
    target_link_libraries (NeuroProofRag ${NEUROPROOF_EXT_LIBS})
    add_dependencies (NeuroProofRag ${NEUROPROOF_DEPS})
    set_target_properties(NeuroProofRag PROPERTIES COMPILE_DEFINITIONS "SETPYTHON")

    get_target_property (NeuroProofPriority_lib NeuroProofPriority LOCATION)
    get_target_property (NeuroProofRag_lib NeuroProofRag LOCATION)

    # copy executables
    add_custom_command (
        TARGET NeuroProofPriority
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofPriority_lib} ${BUILDEM_DIR}/lib)

    add_custom_command (
        TARGET NeuroProofRag
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofRag_lib} ${BUILDEM_DIR}/lib)

    add_custom_command (
        TARGET neuroproof_graph_analyze 
        POST_BUILD
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${neuroproof_graph_analyze_exe} ${BUILDEM_DIR}/bin)

    add_custom_command (
        TARGET neuroproof_graph_analyze_gt 
        POST_BUILD
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${neuroproof_graph_analyze_gt_exe} ${BUILDEM_DIR}/bin)

    add_custom_command (
        TARGET neuroproof_graph_learn 
        POST_BUILD
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${neuroproof_graph_learn_exe} ${BUILDEM_DIR}/bin)
    
    add_custom_command (
        TARGET neuroproof_graph_predict 
        POST_BUILD
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${neuroproof_graph_predict_exe} ${BUILDEM_DIR}/bin)


    # ctests
    enable_testing()

    add_test("test1_sample1_fullpredict"
        ${PYTHON_EXE}
        ${CMAKE_SOURCE_DIR}/integration_tests/test1.py
        ${BUILDEM_DIR}
        ${CMAKE_SOURCE_DIR}
    )

    add_test("test2_sample1_nooppredict"
        ${PYTHON_EXE}
        ${CMAKE_SOURCE_DIR}/integration_tests/test2.py
        ${BUILDEM_DIR}
        ${CMAKE_SOURCE_DIR}
    )

    add_test("test3_sample1_fullanalyze"
        ${PYTHON_EXE}
        ${CMAKE_SOURCE_DIR}/integration_tests/test3.py
        ${BUILDEM_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    
    add_test("test4_sample1_fullanalyzegt"
        ${PYTHON_EXE}
        ${CMAKE_SOURCE_DIR}/integration_tests/test4.py
        ${BUILDEM_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    
    ###############################################################################
endif()
