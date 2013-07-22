CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (FeatureManaager)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")

set (SOURCES FeatureMgr.cpp Features.cpp)

add_library (FeatureManager SHARED ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    add_dependencies (FeatureManager ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
endif()
