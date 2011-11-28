CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (NeuroProof)


add_subdirectory (ImportsExports)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET (CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")

INCLUDE_DIRECTORIES(
    /usr/include/python2.6
)

ADD_EXECUTABLE(NeuroProof_test NeuroProof_test.cpp)
TARGET_LINK_LIBRARIES(NeuroProof_test ImportsExports jsonlib boost_thread-mt)
get_target_property (EXE_LOC NeuroProof_test LOCATION)

ADD_LIBRARY(NeuroProofPriority SHARED pythonNeuroProofPriorityInterface.cpp)
TARGET_LINK_LIBRARIES(NeuroProofPriority ImportsExports jsonlib boost_thread-mt boost_python python2.6)

    
ADD_LIBRARY(NeuroProofRag SHARED pythonRagInterface.cpp)
TARGET_LINK_LIBRARIES(NeuroProofRag ImportsExports jsonlib boost_thread-mt boost_python python2.6)



add_custom_command (
    TARGET NeuroProof_test
    POST_BUILD
    COMMAND mkdir
    ARGS -p ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${EXE_LOC} ${CMAKE_SOURCE_DIR}/bin
    )

