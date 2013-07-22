CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Rag)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (SOURCES RagIO.cpp RagUtils.cpp)

add_library (Rag SHARED ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    add_dependencies (Rag ${jsoncpp_NAME} ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
endif()

add_executable (basic_rag_test unit_tests/basic_rag.cpp)

if (NOT BUILDEM_DIR STREQUAL "None")
    add_dependencies (basic_rag_test ${jsoncpp_NAME} ${boost_NAME})
endif()

target_link_libraries (basic_rag_test ${json_LIB} ${boost_LIBS} ${PYTHON_LIBRARY_FILE} Rag)

get_target_property (basic_rag_test_exe basic_rag_test LOCATION)

if (NOT ${CMAKE_SOURCE_DIR} STREQUAL ${BUILDLOC})  
    add_custom_command (
        TARGET basic_rag_test 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${basic_rag_test_exe} ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E remove ${basic_rag_test_exe})
endif()

add_test ("simple_rag_unit_tests" ${CMAKE_SOURCE_DIR}/bin/basic_rag_test)
