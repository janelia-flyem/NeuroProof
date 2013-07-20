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

    enable_testing()
    add_subdirectory (Rag)
    add_subdirectory (Stack)
    add_subdirectory (EdgeEditor)
    add_subdirectory (BioPriors)
    add_subdirectory (FeatureManager)
    add_subdirectory (Algorithms)
    add_subdirectory (Classifier)

    set (NEUROPROOF_INT_LIBS Rag Stack EdgeEditor BioPriors FeatureManager Algorithms Classifier)
    set (NEUROPROOF_EXT_LIBS ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} ${boost_LIBS} ${PYTHON_LIBRARY_FILE})
    set (NEUROPROOF_DEPS ${jsoncpp_NAME} ${boost_NAME} ${hdf5_NAME} ${vigra_NAME} ${opencv_NAME})

    include_directories (BEFORE ${BUILDEM_DIR}/include ${PYTHON_INCLUDE_PATH})

    # Handle all of NeuroProof sources and dependent code
    add_executable (neuroproof_graph_analyze src/neuroproof_graph_analyze.cpp)
    add_executable (neuroproof_graph_analyze_gt src/neuroproof_graph_analyze_gt.cpp)
    add_executable (neuroproof_graph_learn src/neuroproof_graph_learn.cpp)
    add_executable (neuroproof_graph_predict src/neuroproof_graph_predict.cpp)
    add_executable (neuroproof_create_spgraph src/neuroproof_create_spgraph.cpp)

    target_link_libraries (neuroproof_graph_analyze ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    target_link_libraries (neuroproof_graph_analyze_gt ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    target_link_libraries (neuroproof_graph_learn ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    target_link_libraries (neuroproof_graph_predict ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    target_link_libraries (neuroproof_create_spgraph ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    
    add_dependencies (neuroproof_graph_analyze ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_analyze_gt ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_learn ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_predict ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_create_spgraph ${NEUROPROOF_DEPS})

    get_target_property (neuroproof_graph_analyze_exe neuroproof_graph_analyze LOCATION)
    get_target_property (neuroproof_graph_analyze_gt_exe neuroproof_graph_analyze_gt LOCATION)
    get_target_property (neuroproof_graph_learn_exe neuroproof_graph_learn LOCATION)
    get_target_property (neuroproof_graph_predict_exe neuroproof_graph_predict LOCATION)
    get_target_property (neuroproof_create_spgraph_exe neuroproof_create_spgraph LOCATION)

    add_library (NeuroProofPriority SHARED src/pythonNeuroProofPriorityInterface.cpp EdgeEditor/EdgeEditor.cpp EdgeEditor/NodeCentricRank.cpp EdgeEditor/NodeSizeRank.cpp EdgeEditor/OrphanRank.cpp EdgeEditor/SynapseRank.cpp EdgeEditor/ProbEdgeRank.cpp)
    target_link_libraries (NeuroProofPriority ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
    add_dependencies (NeuroProofPriority ${NEUROPROOF_DEPS})

    # need to build components separately to set python flag
    add_library (NeuroProofRag SHARED src/pythonRagInterface.cpp Rag/RagIO.cpp FeatureManager/FeatureMgr.cpp FeatureManager/Features.cpp Algorithms/MergePriorityFunction.cpp Algorithms/BatchMergeMRFh.cpp Rag/RagUtils.cpp Stack/StackController.cpp Stack/VolumeLabelData.cpp BioPriors/StackAgglomAlgs.cpp)
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
        COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofPriority_lib} ${BUILDEM_DIR}/lib/python2.7/site-packages)

    add_custom_command (
        TARGET NeuroProofRag
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofRag_lib} ${BUILDEM_DIR}/lib/python2.7/site-packages)

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

    add_custom_command (
        TARGET neuroproof_create_spgraph
        POST_BUILD
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${neuroproof_create_spgraph_exe} ${BUILDEM_DIR}/bin)



    # ctests

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
   
    add_test("test5_sample1_graphtrain"
        ${PYTHON_EXE}
        ${CMAKE_SOURCE_DIR}/integration_tests/test5.py
        ${BUILDEM_DIR}
        ${CMAKE_SOURCE_DIR}
    )

    add_test("test_rag_python"
        ${PYTHON_EXE}
        ${CMAKE_SOURCE_DIR}/integration_tests/testragscript.py
        ${CMAKE_SOURCE_DIR}/integration_tests/temp_data/temp_rag.json
    )

    add_test("test_priority_python"
        ${PYTHON_EXE}
        ${CMAKE_SOURCE_DIR}/integration_tests/testpriority.py
        ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_graph_processed.json
        ${CMAKE_SOURCE_DIR}/integration_tests/temp_data/proofread_samp1_graph.json
        ${CMAKE_SOURCE_DIR}/integration_tests/outputs/proofread_samp1_graph.json
    )

    ###############################################################################
endif()
