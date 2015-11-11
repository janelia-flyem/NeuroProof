CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (BioPriors)

set (SOURCES BioStack.cpp StackAgglomAlgs.cpp StackLearnAlgs.cpp IterativeLearn.cpp Dataset.cpp IterativeLearn_semi.cpp)

include_directories (AFTER ${CMAKE_SOURCE_DIR}/src/external_packages)

if (APPLE) 
	add_library (BioPriors ${SOURCES})
else()
	add_library (BioPriors SHARED ${SOURCES})
endif()	

install (TARGETS BioPriors DESTINATION lib${LIB_SUFFIX})
