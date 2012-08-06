CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (NeuroProof)

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

add_subdirectory (ImportsExports)
add_subdirectory (FeatureManager)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET (CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")


ADD_EXECUTABLE(NeuroProof_test NeuroProof_test.cpp)
ADD_EXECUTABLE(calculate_GPR calculate_GPR.cpp)
TARGET_LINK_LIBRARIES(NeuroProof_test ImportsExports jsonlib boost_thread-mt)
TARGET_LINK_LIBRARIES(calculate_GPR ImportsExports jsonlib boost_thread-mt)
get_target_property (EXE_LOC NeuroProof_test LOCATION)
get_target_property (EXE_LOC2 calculate_GPR LOCATION)

ADD_LIBRARY(NeuroProofPriority SHARED pythonNeuroProofPriorityInterface.cpp)
TARGET_LINK_LIBRARIES(NeuroProofPriority ImportsExports FeatureManager jsonlib boost_thread-mt boost_python)


ADD_LIBRARY(NeuroProofRag SHARED pythonRagInterface.cpp ImportsExports/ImportExportRagPriority.cpp FeatureManager/FeatureManager.cpp)
TARGET_LINK_LIBRARIES(NeuroProofRag jsonlib boost_thread-mt boost_python)
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
    TARGET calculate_GPR 
    POST_BUILD
    COMMAND mkdir
    ARGS -p ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${EXE_LOC2} ${CMAKE_SOURCE_DIR}/bin
    )
