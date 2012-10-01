CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (ImportsExports)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (CMAKE_CXX_FLAGS_RELEASE "-O3")
set (CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set (CMAKE_CXX_LINK_FLAGS "-O3 ${em_LIB_DIRS}")
set (CMAKE_DEBUG_POSTFIX "-g")

set (SOURCES ImportExportRagPriority.cpp)

add_library (ImportsExports SHARED ${SOURCES})
add_dependencies (ImportsExports jsoncpp)
