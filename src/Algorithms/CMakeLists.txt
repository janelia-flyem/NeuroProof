CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Algorithms)

set (SOURCES BatchMergeMRFh.cpp MergePriorityFunction.cpp)

if (APPLE) 
	add_library (Algorithms ${SOURCES})
else()
	add_library (Algorithms SHARED ${SOURCES})
endif()	

# include vigra directories if none are built or included in the system path
include_directories (AFTER ${MAKE_CURRENT_BINARY_DIR}/src/external_packages)

install (TARGETS Algorithms DESTINATION lib${LIB_SUFFIX})
