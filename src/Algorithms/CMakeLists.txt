CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Algorithms)

set (SOURCES BatchMergeMRFh.cpp MergePriorityFunction.cpp)
add_library (Algorithms ${SOURCES})

# include vigra directories if none are built or included in the system path
include_directories (AFTER ${MAKE_CURRENT_BINARY_DIR}/src/external_packages)

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    add_dependencies (Algorithms ${boost_NAME})
endif()

install (TARGETS Algorithms DESTINATION lib${LIB_SUFFIX})
