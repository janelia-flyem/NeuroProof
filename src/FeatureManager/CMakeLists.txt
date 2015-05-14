CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (FeatureManaager)

set (SOURCES FeatureMgr.cpp Features.cpp)
add_library (FeatureManager SHARED ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    add_dependencies (FeatureManager ${boost_NAME})
endif()
