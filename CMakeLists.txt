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

    add_subdirectory (ImportsExports)
    add_subdirectory (FeatureManager)

    include_directories (BEFORE ${BUILDEM_DIR}/include ${PYTHON_INCLUDE_PATH})

    # Handle all of NeuroProof sources and dependent code
    add_executable (analyze_segmentation_graph analyze_segmentation_graph.cpp)
#target_link_libraries (NeuroProof_test ImportsExports ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} boost_thread boost_system)
    target_link_libraries (analyze_segmentation_graph ImportsExports ${json_LIB} boost_thread boost_system boost_program_options)
    #add_dependencies (NeuroProof_test ${jsoncpp_NAME} ${boost_NAME} ${hdf5_NAME} ${vigra_NAME} ${opencv_NAME})
    add_dependencies (analyze_segmentation_graph ${jsoncpp_NAME} ${boost_NAME})

    get_target_property (analyze_segmentation_graph_exe analyze_segmentation_graph LOCATION)

    add_library (NeuroProofPriority SHARED pythonNeuroProofPriorityInterface.cpp)
    target_link_libraries (NeuroProofPriority ImportsExports FeatureManager ${json_LIB} boost_thread boost_python)
    add_dependencies (NeuroProofPriority ${python_NAME} ${jsoncpp_NAME} ${boost_NAME})

    add_library (NeuroProofRag SHARED pythonRagInterface.cpp ImportsExports/ImportExportRagPriority.cpp FeatureManager/FeatureManager.cpp)
    target_link_libraries (NeuroProofRag ${json_LIB} boost_thread boost_python)
    add_dependencies (NeuroProofRag ${python_NAME} ${jsoncpp_NAME} ${boost_NAME})
    set_target_properties(NeuroProofRag PROPERTIES COMPILE_DEFINITIONS "SETPYTHON")

    get_target_property (NeuroProofPriority_lib NeuroProofPriority LOCATION)
    get_target_property (NeuroProofRag_lib NeuroProofRag LOCATION)

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
        TARGET analyze_segmentation_graph 
        POST_BUILD
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${analyze_segmentation_graph_exe} ${BUILDEM_DIR}/bin)

    ###############################################################################
endif()
