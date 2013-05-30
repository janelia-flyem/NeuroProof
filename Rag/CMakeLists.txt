CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Rag)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (SOURCES RagIO.cpp RagUtils.cpp)

include_directories (BEFORE ${FLYEM_BUILD_DIR}/include)

add_library (Rag SHARED ${SOURCES})
add_dependencies (Rag ${jsoncpp_NAME} ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
