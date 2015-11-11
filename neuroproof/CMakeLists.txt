CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

enable_testing()

if (ENABLE_GUI)
    add_subdirectory (../src/Gui "${CMAKE_CURRENT_BINARY_DIR}/Gui")
    add_subdirectory (../src/StackGui "${CMAKE_CURRENT_BINARY_DIR}/StackGui")
endif()


# Handle all of NeuroProof sources and dependent code
add_executable (neuroproof_graph_analyze neuroproof_graph_analyze.cpp)
# add_executable (neuroproof_graph_build_dvid neuroproof_graph_build_dvid.cpp)
add_executable (neuroproof_graph_build_stream neuroproof_graph_build_stream.cpp)
# add_executable (neuroproof_agg_prob_dvid neuroproof_agg_prob_dvid.cpp)
add_executable (neuroproof_graph_analyze_gt neuroproof_graph_analyze_gt.cpp)
add_executable (neuroproof_graph_learn neuroproof_graph_learn.cpp)
add_executable (neuroproof_graph_predict neuroproof_graph_predict.cpp)
add_executable (neuroproof_create_spgraph neuroproof_create_spgraph.cpp)
if (ENABLE_GUI)
    add_executable (neuroproof_stack_viewer neuroproof_stack_viewer.cpp)
endif()

add_library (NeuroProofPriority SHARED pythonNeuroProofPriorityInterface.cpp
    ../src/EdgeEditor/EdgeEditor.cpp ../src/EdgeEditor/NodeCentricRank.cpp
    ../src/EdgeEditor/NodeSizeRank.cpp ../src/EdgeEditor/OrphanRank.cpp
    ../src/EdgeEditor/SynapseRank.cpp ../src/EdgeEditor/ProbEdgeRank.cpp
    ../src/IO/RagIO.cpp ../src/Rag/RagUtils.cpp)
add_library (NeuroProofRag SHARED pythonRagInterface.cpp ../src/IO/RagIO.cpp
    ../src/FeatureManager/FeatureMgr.cpp ../src/FeatureManager/Features.cpp
    ../src/Algorithms/MergePriorityFunction.cpp
    ../src/Algorithms/BatchMergeMRFh.cpp ../src/Rag/RagUtils.cpp
    ../src/Stack/Stack.cpp ../src/Stack/VolumeLabelData.cpp ../src/BioPriors/StackAgglomAlgs.cpp)


target_link_libraries (neuroproof_graph_analyze ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
# target_link_libraries (neuroproof_graph_build_dvid ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_build_stream ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
# target_link_libraries (neuroproof_agg_prob_dvid ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_analyze_gt ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_learn ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_predict ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_create_spgraph ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
if (ENABLE_GUI)
    target_link_libraries (neuroproof_stack_viewer ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
endif()
target_link_libraries (NeuroProofPriority ${json_LIB} ${boost_LIBS} ${PYTHON_LIBRARY_FILE})
target_link_libraries (NeuroProofRag ${NEUROPROOF_EXT_LIBS})

# need to build components separately to set python flag
set_target_properties(NeuroProofRag PROPERTIES COMPILE_DEFINITIONS "SETPYTHON")

# install python scripts and supporting libraries

if (APPLE)
    set_target_properties( NeuroProofPriority PROPERTIES SUFFIX  ".so")
    set_target_properties( NeuroProofRag PROPERTIES SUFFIX  ".so")
endif()

install(TARGETS NeuroProofPriority 
        DESTINATION ${NEUROPROOF_PYTHON_INSTALL_DIR})

install(TARGETS NeuroProofRag
        DESTINATION ${NEUROPROOF_PYTHON_INSTALL_DIR})

# install executables
install (TARGETS neuroproof_graph_analyze DESTINATION bin)
# install (TARGETS neuroproof_graph_build_dvid DESTINATION bin)
install (TARGETS neuroproof_graph_build_stream DESTINATION bin)
# install (TARGETS neuroproof_agg_prob_dvid DESTINATION bin)
install (TARGETS neuroproof_graph_analyze_gt DESTINATION bin)
install (TARGETS neuroproof_graph_learn DESTINATION bin)
install (TARGETS neuroproof_graph_predict DESTINATION bin)
install (TARGETS neuroproof_create_spgraph DESTINATION bin)

if (ENABLE_GUI)
    install (TARGETS neuroproof_stack_viewer DESTINATION bin)
endif()
# ctests

enable_testing()
add_test("test1_sample1_fullpredict"
    ${PYTHON_EXE}
    ${CMAKE_SOURCE_DIR}/integration_tests/test1.py
    ${BUILDLOC}
    ${CMAKE_SOURCE_DIR}
)

add_test("test2_sample1_nooppredict"
    ${PYTHON_EXE}
    ${CMAKE_SOURCE_DIR}/integration_tests/test2.py
    ${BUILDLOC}
    ${CMAKE_SOURCE_DIR}
)

# TODO: make separate test for APPLE environment (some instabilities in RAND)
if (NOT APPLE)
	add_test("test3_sample1_fullanalyze"
	    ${PYTHON_EXE}
	    ${CMAKE_SOURCE_DIR}/integration_tests/test3.py
	    ${BUILDLOC}
	    ${CMAKE_SOURCE_DIR}
	)

	add_test("test4_sample1_fullanalyzegt"
	    ${PYTHON_EXE}
	    ${CMAKE_SOURCE_DIR}/integration_tests/test4.py
	    ${BUILDLOC}
	    ${CMAKE_SOURCE_DIR}
	)
endif()

add_test("test5_sample1_graphtrain"
    ${PYTHON_EXE}
    ${CMAKE_SOURCE_DIR}/integration_tests/test5.py
    ${BUILDLOC}
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

