CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (EDGEEDITOR)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)



set (SOURCES EdgeRankToufiq.cpp EdgeEditor.cpp NodeCentricRank.cpp NodeSizeRank.cpp OrphanRank.cpp SynapseRank.cpp ProbEdgeRank.cpp GPR.cpp )

add_library (EdgeEditor SHARED ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    add_dependencies (EdgeEditor ${jsoncpp_NAME} ${boost_NAME})
endif()

