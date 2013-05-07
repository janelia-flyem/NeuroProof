CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (ImportsExports)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (SOURCES ImportExportRagPriority.cpp)

include_directories (BEFORE ${FLYEM_BUILD_DIR}/include ${PYTHON_INCLUDE_PATH})

add_library (ImportsExports SHARED ${SOURCES})
add_dependencies (ImportsExports ${jsoncpp_NAME} ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
