CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (EDGEEDITOR)

set (SOURCES EdgeRankToufiq.cpp EdgeEditor.cpp NodeCentricRank.cpp NodeSizeRank.cpp OrphanRank.cpp SynapseRank.cpp ProbEdgeRank.cpp GPR.cpp )

include_directories (AFTER ${CMAKE_SOURCE_DIR}/src/external_packages)

add_library (EdgeEditor ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    include (jsoncpp)
    include (opencv)
    add_dependencies (EdgeEditor ${jsoncpp_NAME} ${boost_NAME} ${opencv_NAME})
endif()

install (TARGETS EdgeEditor DESTINATION lib${LIB_SUFFIX})
