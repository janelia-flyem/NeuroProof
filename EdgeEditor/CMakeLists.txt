CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (EDGEEDITOR)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (SOURCES EdgeEditor.cpp NodeEdgeRank.cpp NodeSizeRank.cpp OrphanRank.cpp SynapseRank.cpp ProbEdgeRank.cpp)

# TODO: remove explicit dependency on buildem 
include_directories (BEFORE ${BUILDEM_DIR}/include ${PYTHON_INCLUDE_PATH})

add_library (EdgeEditor SHARED ${SOURCES})
add_dependencies (EdgeEditor ${jsoncpp_NAME} ${boost_NAME})

