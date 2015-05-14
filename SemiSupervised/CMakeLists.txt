CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (SemiSupervised)

set (SOURCES kmeans.cpp weightmatrix1.cpp)

include_directories (${CMAKE_SOURCE_DIR}/external_packages)

add_library (SemiSupervised SHARED ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    include (vigra)
    add_dependencies (SemiSupervised ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
endif()

