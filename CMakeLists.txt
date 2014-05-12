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
    message ("WARNING: To use Buildem, Buildem directory (for all downloads & builds) should be specified via -DBUILDEM_DIR=<path> on cmake command line.")
    message ("Builds will be placed here: ${CMAKE_SOURCE_DIR}/bin")
else()
    message ("FlyEM downloads and builds will be placed here: ${BUILDEM_DIR}")
endif ()
###############################################################################

if (${BUILDEM_DIR} STREQUAL "None")
    set (BUILDLOC ${CMAKE_SOURCE_DIR})
    set (PYLIBLOC ${CMAKE_SOURCE_DIR}/lib)
else()
    set (BUILDLOC ${BUILDEM_DIR})
    set (PYLIBLOC ${BUILDEM_DIR}/lib/python2.7/site-packages)
endif()

if (NOT ${BUILDEM_DIR} STREQUAL "None")
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
        return()
    endif() 
    ###############################################################################

    # Use modules from the downloaded buildem
    set (CMAKE_MODULE_PATH ${BUILDEM_REPO_DIR})
    message("Using cmake modules from ${BUILDEM_REPO_DIR}")


    # check if GUI is disabled
    if (NOT DISABLE_GUI)
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


    # disable vigra numpy build
    SET(DISABLE_VIGRANUMPY "TRUE")

    # Download and compile dependencies -- dependencies need to be built manually if BUILDEM_DIR is not set
    if (NOT DISABLE_GUI)
        include (qt4)
        include (vtk)
    endif()

    include (hdf5)
    include (python)
    include (jsoncpp)
    include (boost)
    include (vigra)
    include (opencv)

    if (NOT DISABLE_GUI)
        set (NEUROPROOF_DEPS ${vtk_NAME} ${qt4_NAME} ${jsoncpp_NAME} ${boost_NAME} ${hdf5_NAME} ${vigra_NAME} ${opencv_NAME} RescanQt4)
    else()
        set (NEUROPROOF_DEPS ${vtk_NAME} ${qt4_NAME} ${jsoncpp_NAME} ${boost_NAME} ${hdf5_NAME} ${vigra_NAME} ${opencv_NAME})
    endif()

    set (boost_LIBS ${boost_LIBS} ${BUILDEM_LIB_DIR}/libboost_unit_test_framework.so)

    if (NOT DISABLE_GUI)
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
    FIND_PACKAGE(PythonLibs)
    
    if (NOT DISABLE_GUI)
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
    set (boost_LIBS boost_thread-mt boost_system boost_program_options boost_python boost_unit_test_framework boost_filesystem)
    
    if (NOT DISABLE_GUI)
        set (vtk_LIBS vtkHybrid vtkRendering vtkVolumeRendering vtkWidgets vtkCommon QVTK)
        set (qt_LIBS  ${QT_LIBRARIES})
    endif()
    
    set (PYTHON_LIBRARY_FILE ${PYTHON_LIBRARIES})
    set (PYTHON_EXE "python") 
endif (NOT ${BUILDEM_DIR} STREQUAL "None")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILDLOC}/bin)
if (NOT EXISTS ${BUILDLOC}/bin)
    file (MAKE_DIRECTORY ${BUILDLOC}/bin)
endif() 

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILDLOC}/lib)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILDLOC}/lib)
if (NOT EXISTS ${BUILDLOC}/lib)
    file (MAKE_DIRECTORY ${BUILDLOC}/lib)
endif() 

# Compile NeuroProof components
include_directories (BEFORE ${PYTHON_INCLUDE_PATH})
include_directories (${CMAKE_SOURCE_DIR}/SemiSupervised)

enable_testing()
add_subdirectory (Rag)
add_subdirectory (Stack)
add_subdirectory (EdgeEditor)
add_subdirectory (BioPriors)
add_subdirectory (FeatureManager)
add_subdirectory (Algorithms)
add_subdirectory (Classifier)
add_subdirectory (SemiSupervised)

if (NOT DISABLE_GUI)
    add_subdirectory (Gui)
    set (NEUROPROOF_INT_LIBS Rag Stack EdgeEditor Gui BioPriors FeatureManager Algorithms Classifier SemiSupervised)
    set (NEUROPROOF_EXT_LIBS ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} ${boost_LIBS} ${PYTHON_LIBRARY_FILE} ${vtk_LIBS} ${qt_LIBS})
else()
    set (NEUROPROOF_INT_LIBS Rag Stack EdgeEditor BioPriors FeatureManager Algorithms Classifier SemiSupervised)
    set (NEUROPROOF_EXT_LIBS ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} ${boost_LIBS} ${PYTHON_LIBRARY_FILE})
endif()


# Handle all of NeuroProof sources and dependent code
add_executable (neuroproof_graph_analyze src/neuroproof_graph_analyze.cpp)
add_executable (neuroproof_graph_analyze_gt src/neuroproof_graph_analyze_gt.cpp)
add_executable (neuroproof_graph_learn src/neuroproof_graph_learn.cpp)
add_executable (neuroproof_graph_predict src/neuroproof_graph_predict.cpp)
add_executable (neuroproof_create_spgraph src/neuroproof_create_spgraph.cpp)
if (NOT DISABLE_GUI)
    add_executable (neuroproof_stack_viewer src/neuroproof_stack_viewer.cpp)
endif()

add_library (NeuroProofPriority SHARED src/pythonNeuroProofPriorityInterface.cpp EdgeEditor/EdgeEditor.cpp EdgeEditor/NodeCentricRank.cpp EdgeEditor/NodeSizeRank.cpp EdgeEditor/OrphanRank.cpp EdgeEditor/SynapseRank.cpp EdgeEditor/ProbEdgeRank.cpp)
add_library (NeuroProofRag SHARED src/pythonRagInterface.cpp Rag/RagIO.cpp FeatureManager/FeatureMgr.cpp FeatureManager/Features.cpp Algorithms/MergePriorityFunction.cpp Algorithms/BatchMergeMRFh.cpp Rag/RagUtils.cpp Stack/Stack.cpp Stack/VolumeLabelData.cpp BioPriors/StackAgglomAlgs.cpp)

target_link_libraries (neuroproof_graph_analyze ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_analyze_gt ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_learn ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_graph_predict ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (neuroproof_create_spgraph ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
if (NOT DISABLE_GUI)
    target_link_libraries (neuroproof_stack_viewer ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
endif()
target_link_libraries (NeuroProofPriority ${NEUROPROOF_INT_LIBS} ${NEUROPROOF_EXT_LIBS})
target_link_libraries (NeuroProofRag ${NEUROPROOF_EXT_LIBS})

if (NOT ${BUILDEM_DIR} STREQUAL "None")
    add_dependencies (neuroproof_graph_analyze ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_analyze_gt ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_learn ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_graph_predict ${NEUROPROOF_DEPS})
    add_dependencies (neuroproof_create_spgraph ${NEUROPROOF_DEPS})
    if (NOT DISABLE_GUI)
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

