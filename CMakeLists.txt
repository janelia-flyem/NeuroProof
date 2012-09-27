CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project (NeuroProof)

include (ExternalProject)

FIND_PACKAGE(PythonLibs)
include_directories (${PYTHON_INCLUDE_PATH})

set (FLYEM_GIT_REPOS "/groups/flyem/proj/code/git-repos")

# Setup FlyEM build caches where we can download code hopefully once
set (em_INSTALL_DIR None CACHE STRING "Select prefix path for CMake installs/downloads")
if (em_INSTALL_DIR STREQUAL "None")
if ("$ENV{USERNAME}" STREQUAL "")
    if ("$ENV{USER}" STREQUAL "")
        if ("$ENV{LOGNAME}" STREQUAL "")
            message (FATAL_ERROR "Must have one of environment variables USERNAME, USER, or LOGNAME set to user")
        else ()
            set (USERNAME "$ENV{LOGNAME}")
        endif ()
    else ()
        set (USERNAME "$ENV{USER}")
    endif()
else ()
    set (USERNAME "$ENV{USERNAME}")
endif ()

if (EXISTS /groups/flyem/home/${USERNAME})
    set (em_INSTALL_DIR  "/groups/flyem/home/${USERNAME}/builds")
else ()
    set (em_INSTALL_DIR  "${CMAKE_CURRENT_BINARY_DIR}/builds")
endif ()
endif ()
message("Installation directory for ${PROJECT_NAME} include files and libraries: ${em_INSTALL_DIR}")

set (RUN_ENVIRONMENT "Workstation" CACHE TYPE STRING)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (MAC 1)
    set (system_type "Mac")
    message ("${PROJECT_NAME}: Build system is MacOS X")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message (FATAL_ERROR "${PROJECT_NAME}: Build system is Windows -- NOT SUPPORTED!!")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (MAC 0)
    if ("$ENV{SGE_CLUSTER_NAME}" STREQUAL "janelia1")
        message ("${PROJECT_NAME}: Build system is Cluster Linux -- detected SGE_CLUSTER_NAME=janelia")
        set (system_type "Cluster")
    else ()
        message ("${PROJECT_NAME}: Build system is Linux -- not clustered")
        set (system_type "Linux")
    endif ()
else ()
    set (MAC 0)
    message ("${PROJECT_NAME}: Build system is ${CMAKE_SYSTEM_NAME} -- don't know if it'll work!")
    set (system_type "${CMAKE_SYSTEM_NAME}")
endif ()

set (em_ARCH_INSTALL_DIR  "${em_INSTALL_DIR}/${system_type}")
set (em_INSTALL_PREFIX    "--prefix=${em_ARCH_INSTALL_DIR}")
set (em_LIB_DIRS "-L${em_ARCH_INSTALL_DIR}/lib -L/usr/local/lib")
message ("Shared libraries will be placed here: ${em_ARCH_INSTALL_DIR}/lib")
## -- finish with FlyEM build cache spec

add_subdirectory (ImportsExports)
add_subdirectory (FeatureManager)

IF(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (CMAKE_CXX_FLAGS_RELEASE "-O3")
set (CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set (CMAKE_CXX_LINK_FLAGS "-O3 ${em_LIB_DIRS}")
set (CMAKE_DEBUG_POSTFIX "-g")

# Download and compile jsoncpp dependency
exec_program (g++ ARGS "-dumpversion" OUTPUT_VARIABLE GCC_VERSION)
set (jsoncpp_RELEASE  0.5.0)
set (jsoncpp_NAME     "jsoncpp-src-${jsoncpp_RELEASE}")

macro (set_src_dir TARGET_ABBREV TARGET_NAME)
set (${TARGET_NAME}_SRC_DIR   "${em_ARCH_INSTALL_DIR}/src/${TARGET_NAME}")
set (${TARGET_ABBREV}_SRC_DIR "${em_ARCH_INSTALL_DIR}/src/${TARGET_NAME}")
endmacro (set_src_dir)

set_src_dir(jsoncpp ${jsoncpp_NAME})
#include_directories (${jsoncpp_SRC_DIR}/include/json)
message ("Including JSON include files from here: ${jsoncpp_SRC_DIR}/include/json")

ExternalProject_Add(${jsoncpp_NAME}
    PREFIX ${em_ARCH_INSTALL_DIR}
    GIT_REPOSITORY ${FLYEM_GIT_REPOS}/jsoncpp.git
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND python scons.py platform=linux-gcc
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)
add_custom_command (TARGET ${jsoncpp_NAME}
                    POST_BUILD
                    COMMAND cp ${jsoncpp_SRC_DIR}/libs/linux-gcc-*/libjson_linux-gcc-*_libmt.so ${em_ARCH_INSTALL_DIR}/lib/libjsoncpp.so
                    COMMENT "Copied jsoncpp library to ${em_ARCH_INSTALL_DIR}/lib")
set (json_LIB ${em_ARCH_INSTALL_DIR}/lib/libjsoncpp.so)

#add_library (jsoncpp STATIC IMPORTED)
#add_dependencies (jsoncpp ${jsoncpp_NAME})
#set_property (TARGET jsoncpp PROPERTY
#            IMPORTED_LOCATION ${jsoncpp_SRC_DIR}/libs/linux-gcc-${GCC_VERSION}/libjson_linux-gcc-${GCC_VERSION}_libmt.so)

# Handle all of NeuroProof sources and dependent code
ADD_EXECUTABLE(NeuroProof_test NeuroProof_test.cpp)
ADD_EXECUTABLE(generate_stats generate_stats.cpp)
ADD_EXECUTABLE(calculate_GPR calculate_GPR.cpp)
TARGET_LINK_LIBRARIES(NeuroProof_test ImportsExports ${json_LIB} boost_thread-mt)
TARGET_LINK_LIBRARIES(generate_stats ImportsExports ${json_LIB} boost_thread-mt)
TARGET_LINK_LIBRARIES(calculate_GPR ImportsExports ${json_LIB} boost_thread-mt)
add_dependencies (NeuroProof_test ${jsoncpp_NAME})
add_dependencies (generate_stats ${jsoncpp_NAME})
add_dependencies (calculate_GPR ${jsoncpp_NAME})
get_target_property (EXE_LOC NeuroProof_test LOCATION)
get_target_property (EXE_LOC1 generate_stats LOCATION)
get_target_property (EXE_LOC2 calculate_GPR LOCATION)

ADD_LIBRARY(NeuroProofPriority SHARED pythonNeuroProofPriorityInterface.cpp)
TARGET_LINK_LIBRARIES(NeuroProofPriority ImportsExports FeatureManager ${json_LIB} boost_thread-mt boost_python)
add_dependencies (NeuroProofPriority ${jsoncpp_NAME})


ADD_LIBRARY(NeuroProofRag SHARED pythonRagInterface.cpp ImportsExports/ImportExportRagPriority.cpp FeatureManager/FeatureManager.cpp)
TARGET_LINK_LIBRARIES(NeuroProofRag ${json_LIB} boost_thread-mt boost_python)
add_dependencies (NeuroProofRag ${jsoncpp_NAME})
set_target_properties(NeuroProofRag PROPERTIES COMPILE_DEFINITIONS "SETPYTHON")

add_custom_command (
    TARGET NeuroProof_test
    POST_BUILD
    COMMAND mkdir
    ARGS -p ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${EXE_LOC} ${CMAKE_SOURCE_DIR}/bin
    )

add_custom_command (
    TARGET generate_stats
    POST_BUILD
    COMMAND mkdir
    ARGS -p ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${EXE_LOC1} ${CMAKE_SOURCE_DIR}/bin
    )

add_custom_command (
    TARGET calculate_GPR 
    POST_BUILD
    COMMAND mkdir
    ARGS -p ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${EXE_LOC2} ${CMAKE_SOURCE_DIR}/bin
    )
