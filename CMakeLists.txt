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
# Check if FLYEM_BUILD_DIR has already been assigned.  If not, create a default.
set (FLYEM_BUILD_DIR "None" CACHE TYPE STRING)

if (${FLYEM_BUILD_DIR} STREQUAL "None")
    message (FATAL_ERROR "ERROR: FlyEM build directory (for all downloads & builds) should be specified via -DFLYEM_BUILD_DIR=<path> on cmake command line.")
endif ()

message ("FlyEM downloads and builds will be placed here: ${FLYEM_BUILD_DIR}")

###############################################################################

###############################################################################
# Download and install flyem-build, if it isn't already in FLYEM_BUILD_DIR.
set (FLYEM_BUILD_REPO_DIR ${FLYEM_BUILD_DIR}/src/flyem-build)
if (NOT EXISTS ${FLYEM_BUILD_REPO_DIR}/python.cmake)
    message ("Installing flyem-build repo...")
    ExternalProject_Add(flyem-build
        PREFIX ${FLYEM_BUILD_DIR}
        GIT_REPOSITORY http://github.com/janelia-flyem/flyem-build.git
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        CONFIGURE_COMMAND "" 
        BUILD_COMMAND ""
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND ""
    )
    message ("\n**********************************************************\n")
    message ("\nAfter running make, you must re-run the cmake command once")
    message ("flyem-build has been downloaded!\n")
    message ("\n***********************************************************\n")
else ()
    ###############################################################################

    # Use modules from the downloaded flyem-build
    set (CMAKE_MODULE_PATH ${FLYEM_BUILD_REPO_DIR})
    message("Using cmake modules from ${FLYEM_BUILD_REPO_DIR}")

    # Download and compile dependencies
    include (python)
    include (jsoncpp)
    include (boost)

    # Compile NeuroProof components

    add_subdirectory (ImportsExports)
    add_subdirectory (FeatureManager)

    include_directories (BEFORE ${FLYEM_BUILD_DIR}/include ${PYTHON_INCLUDE_PATH})

    # Handle all of NeuroProof sources and dependent code
    add_executable (NeuroProof_test NeuroProof_test.cpp)
    add_executable (generate_stats generate_stats.cpp)
    add_executable (calculate_GPR calculate_GPR.cpp)
    target_link_libraries (NeuroProof_test ImportsExports ${json_LIB} boost_thread boost_system)
    target_link_libraries (generate_stats ImportsExports ${json_LIB} boost_thread boost_system)
    target_link_libraries (calculate_GPR ImportsExports ${json_LIB} boost_thread boost_system)
    add_dependencies (NeuroProof_test ${jsoncpp_NAME} ${boost_NAME})
    add_dependencies (generate_stats ${jsoncpp_NAME} ${boost_NAME})
    add_dependencies (calculate_GPR ${jsoncpp_NAME} ${boost_NAME})

    get_target_property (NeuroProof_test_exe NeuroProof_test LOCATION)
    get_target_property (generate_stats_exe generate_stats LOCATION)
    get_target_property (calculate_GPR_exe calculate_GPR LOCATION)

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
        COMMAND ${CMAKE_COMMAND} -E make_directory ${FLYEM_BUILD_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofPriority_lib} ${FLYEM_BUILD_DIR}/lib)

    add_custom_command (
        TARGET NeuroProofRag
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${FLYEM_BUILD_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProofRag_lib} ${FLYEM_BUILD_DIR}/lib)

    add_custom_command (
        TARGET NeuroProof_test
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${FLYEM_BUILD_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${NeuroProof_test_exe} ${FLYEM_BUILD_DIR}/bin)

    add_custom_command (
        TARGET generate_stats
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${FLYEM_BUILD_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${generate_stats_exe} ${FLYEM_BUILD_DIR}/bin)

    add_custom_command (
        TARGET calculate_GPR 
        POST_BUILD
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${FLYEM_BUILD_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${calculate_GPR_exe} ${FLYEM_BUILD_DIR}/bin)

    ###############################################################################
endif()
