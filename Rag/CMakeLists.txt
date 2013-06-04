CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Rag)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (SOURCES RagIO.cpp RagUtils.cpp)

include_directories (BEFORE ${FLYEM_BUILD_DIR}/include)

add_library (Rag SHARED ${SOURCES})
add_dependencies (Rag ${jsoncpp_NAME} ${boost_NAME} ${vigra_NAME} ${opencv_NAME})

add_executable (basic_rag_test unit_tests/basic_rag.cpp)
add_dependencies (basic_rag_test ${jsoncpp_NAME} ${boost_NAME})
#target_link_libraries (basic_rag_test ${json_LIB} ${boost_LIBS} Rag)
target_link_libraries (basic_rag_test ${json_LIB} ${BUILDEM_LIB_DIR}/libboost_unit_test_framework.so Rag)

add_test ("simple_rag_unit_tests" ${BUILDEM_DIR}/bin/basic_rag_test)
