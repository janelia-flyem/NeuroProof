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

if (NOT ${BUILDEM_DIR} STREQUAL "None")
    set (BUILDLOC ${BUILDEM_DIR})
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
else()
    set (BUILDLOC ${CMAKE_SOURCE_DIR})
endif()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILDLOC}/bin)
if (NOT EXISTS ${BUILDLOC}/bin)
    file (MAKE_DIRECTORY ${BUILDLOC}/bin)
endif() 

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILDLOC}/lib)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILDLOC}/lib)
if (NOT EXISTS ${BUILDLOC}/lib)
    file (MAKE_DIRECTORY ${BUILDLOC}/lib)
endif() 

include_directories (BEFORE ${CMAKE_SOURCE_DIR}/src)

# metric library default
set (ENABLE_METRICS YES CACHE BOOL "Build neuroproof metric library")

# default NeuroProof executables
set (ENABLE_NEUROPROOF YES CACHE BOOL "Build Neurproof executables")

# default unit test disable (but always turn on if neuroproof is built)
set (ENABLE_UNITTESTS NO CACHE BOOL "Build Neurproof unittests")

# default gui enable off
set (ENABLE_GUI NO CACHE BOOL "Build GUI for NeuroProof")

if (ENABLE_NEUROPROOF)
    # add the source executables
    add_subdirectory(neuroproof)
endif()

if (ENABLE_NEUROPROOF OR ENABLE_UNITTESTS)
    add_subdirectory(unit_tests)
endif()

if (ENABLE_METRICS)
    add_subdirectory(libMetrics)
endif()


