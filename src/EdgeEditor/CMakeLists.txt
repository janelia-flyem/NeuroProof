CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (EDGEEDITOR)

set (SOURCES EdgeRankToufiq.cpp EdgeEditor.cpp NodeCentricRank.cpp NodeSizeRank.cpp OrphanRank.cpp SynapseRank.cpp ProbEdgeRank.cpp GPR.cpp )

include_directories (AFTER ${CMAKE_SOURCE_DIR}/src/external_packages)

if (APPLE) 
	add_library (EdgeEditor ${SOURCES})
else()
	add_library (EdgeEditor SHARED ${SOURCES})
endif()	

install (TARGETS EdgeEditor DESTINATION lib${LIB_SUFFIX})
