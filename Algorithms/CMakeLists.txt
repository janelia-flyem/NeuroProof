CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Algorithms)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")

set (SOURCES BatchMergeMRFh.cpp MergePriorityFunction.cpp)

include_directories (BEFORE ${FLYEM_BUILD_DIR}/include ${PYTHON_INCLUDE_PATH})

add_library (Algorithms SHARED ${SOURCES})
add_dependencies (Algorithms ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
