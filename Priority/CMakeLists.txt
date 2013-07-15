CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (PRIORITY)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (SOURCES LocalEdgePriority.cpp)

# TODO: remove explicit dependency on buildem 
include_directories (BEFORE ${BUILDEM_DIR}/include ${PYTHON_INCLUDE_PATH})

add_library (Priority SHARED ${SOURCES})
add_dependencies (Priority ${jsoncpp_NAME} ${boost_NAME})

