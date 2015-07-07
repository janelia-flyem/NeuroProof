CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set(ENABLE_COMPILER_H5 TRUE)
set(ENABLE_COMPILER_H5)
set(ENABLE_COMPILER_H5)

if (NOT ${BUILDEM_DIR} STREQUAL "None")
    set (PYLIBLOC ${BUILDEM_DIR}/lib/python2.7/site-packages)
    # check if GUI is disabled
    if (ENABLE_GUI)
        set (QT_QMAKE_EXECUTABLE ${BUILDEM_DIR}/bin/qmake) 
        set (QT_BUILT FALSE)
        if (EXISTS ${QT_QMAKE_EXECUTABLE})
            SET (QT_BUILT TRUE)
        endif()

        if (NOT QT_BUILT)
            include (qt4)
            add_custom_target(RescanQt4 ALL ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR} DEPENDS ${qt4_NAME})
        else()
            add_custom_target(RescanQt4)
            FIND_PACKAGE(Qt4 REQUIRED) 
            include(${QT_USE_FILE}) 
        endif()
    else()
        set (QT_BUILT FALSE)
    endif()

    # Download and compile dependencies -- dependencies need to be built manually if BUILDEM_DIR is not set
    if (ENABLE_GUI)
        include (vtk)
        set (NEUROPROOF_DEPS ${vtk_NAME} ${qt4_NAME} ${jsoncpp_NAME} ${boost_NAME} ${hdf5_NAME} ${vigra_NAME} ${opencv_NAME} ${libdvidcpp_NAME} RescanQt4)
    else()
        set (NEUROPROOF_DEPS ${vtk_NAME} ${qt4_NAME} ${jsoncpp_NAME} ${boost_NAME} ${hdf5_NAME} ${vigra_NAME} ${opencv_NAME} ${libdvidcpp_NAME})
    endif()

    set (boost_LIBS ${boost_LIBS} ${BUILDEM_LIB_DIR}/libboost_unit_test_framework.so)
    set (libdvid_LIBS ${LIBDVIDCPP_LIBRARIES} ${BUILDEM_LIB_DIR}/libpng.so
        ${BUILDEM_LIB_DIR}/libjpeg.so   
        ${BUILDEM_LIB_DIR}/liblz4.so   
        ${BUILDEM_LIB_DIR}/libcurl.so   )

    if (ENABLE_GUI)
        # set to buildem path
        INCLUDE_DIRECTORIES(
                ${BUILDEM_DIR}/include/Qt
                ${BUILDEM_DIR}/include/QtGui
                ${BUILDEM_DIR}/include/QtCore
        )
        
        set (vtk_LIBS 
            ${vtk_LIBPATH}/libQVTK.so
            ${vtk_LIBPATH}/libvtkHybrid.so
            ${vtk_LIBPATH}/libvtkCommon.so
            ${vtk_LIBPATH}/libvtkRendering.so
            ${vtk_LIBPATH}/libvtkVolumeRendering.so
            ${vtk_LIBPATH}/libvtkWidgets.so)
       
        set (qt_LIBS
            ${BUILDEM_DIR}/lib/libQtCore.so
            ${BUILDEM_DIR}/lib/libQtGui.so
        )
    endif()
else ()
    set (PYLIBLOC ${CMAKE_SOURCE_DIR}/lib)
    
    if (ENABLE_GUI)
        SET (QT_BUILT TRUE)
        add_custom_target(RescanQt4)
        FIND_PACKAGE(VTK REQUIRED)
        FIND_PACKAGE(Qt4 REQUIRED)

        INCLUDE(${VTK_USE_FILE})
        IF(NOT VTK_USE_RENDERING)
            MESSAGE(FATAL_ERROR
                "Example requires VTK_USE_RENDERING.")
        ENDIF(NOT VTK_USE_RENDERING)
        INCLUDE(${QT_USE_FILE})
        INCLUDE_DIRECTORIES(
            ${QT_INCLUDE_DIR}
            ${QT_QTGUI_INCLUDE_DIR}
            ${QT_QTCORE_INCLUDE_DIR}
        )
    else()
        SET (QT_BUILT FALSE)
    endif()

    # ensure the libjsoncpp.so is symbolically linked somewhere your lib path
    set (json_LIB jsoncpp)
    set (hdf5_LIBRARIES hdf5 hdf5_hl)
    set (vigra_LIB vigraimpex)
    set (opencv_LIBS opencv_ml opencv_core)
    set (boost_LIBS boost_thread boost_system boost_program_options boost_python boost_unit_test_framework boost_filesystem)
    set (libdvid_LIBS dvidcpp.a  png curl jpeg lz4)

    if (ENABLE_GUI)
        set (vtk_LIBS vtkHybrid vtkRendering vtkVolumeRendering vtkWidgets vtkCommon QVTK)
        set (qt_LIBS  ${QT_LIBRARIES})
    endif()
    
    set (PYTHON_LIBRARY_FILE ${PYTHON_LIBRARIES})
    set (PYTHON_EXE "python") 

endif (NOT ${BUILDEM_DIR} STREQUAL "None")

# Compile NeuroProof components
#include_directories (${CMAKE_SOURCE_DIR}/SemiSupervised)

enable_testing()

if (ENABLE_GUI)
    add_subdirectory (../src/Gui "${CMAKE_CURRENT_BINARY_DIR}/Gui")
    add_subdirectory (../src/StackGui "${CMAKE_CURRENT_BINARY_DIR}/StackGui")
    set (NEUROPROOF_INT_LIBS Rag Stack EdgeEditor Gui BioPriors FeatureManager
        Algorithms Classifier SemiSupervised IO StackGui)
    set (NEUROPROOF_EXT_LIBS ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} ${boost_LIBS} ${libdvid_LIBS} ${PYTHON_LIBRARY_FILE} ${vtk_LIBS} ${qt_LIBS})
else()
    set (NEUROPROOF_INT_LIBS Rag Stack EdgeEditor BioPriors IO FeatureManager Algorithms Classifier SemiSupervised)
    set (NEUROPROOF_EXT_LIBS ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} ${boost_LIBS} ${libdvid_LIBS} ${PYTHON_LIBRARY_FILE})
endif()


# Handle all of NeuroProof sources and dependent code
add_executable (neuroproof_graph_analyze neuroproof_graph_analyze.cpp)
add_executable (neuroproof_graph_build_dvid neuroproof_graph_build_dvid.cpp)
add_executable (neuroproof_graph_build_stream neuroproof_graph_build_stream.cpp)
add_executable (neuroproof_agg_prob_dvid neuroproof_agg_prob_dvid.cpp)
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
target_link_libraries (neuroproof_graph_build_dvid ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_build_stream ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_agg_prob_dvid ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_analyze_gt ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_learn ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_predict ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_create_spgraph ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
if (ENABLE_GUI)
    target_link_libraries (neuroproof_stack_viewer ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
endif()
target_link_libraries (NeuroProofPriority ${json_LIB} ${boost_LIBS} ${PYTHON_LIBRARY_FILE})
target_link_libraries (NeuroProofRag ${NEUROPROOF_EXT_LIBS})

if (NOT ${BUILDEM_DIR} STREQUAL "None")
    add_dependencies (neuroproof_graph_analyze ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_build_dvid ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_build_stream ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_agg_prob_dvid ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_analyze_gt ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_learn ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_predict ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_create_spgraph ${NEUROPROOF_DEPS})
    if (ENABLE_GUI)
        add_dependencies (neuroproof_stack_viewer ${NEUROPROOF_DEPS})
    endif()
    add_dependencies (NeuroProofPriority ${NEUROPROOF_DEPS})
    add_dependencies (NeuroProofRag ${NEUROPROOF_DEPS})
endif()

# need to build components separately to set python flag
set_target_properties(NeuroProofRag PROPERTIES COMPILE_DEFINITIONS "SETPYTHON")

get_target_property (NeuroProofPriority_lib NeuroProofPriority LOCATION)
get_target_property (NeuroProofRag_lib NeuroProofRag LOCATION)


# copy executables
add_custom_command (
    TARGET NeuroProofPriority
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofPriority_lib} ${PYLIBLOC})

add_custom_command (
    TARGET NeuroProofRag
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofRag_lib} ${PYLIBLOC})

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

