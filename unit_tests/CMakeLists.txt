CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (UnitTests)

include_directories (BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory (../Stack "${CMAKE_CURRENT_BINARY_DIR}/Stack")
add_subdirectory (../Rag "${CMAKE_CURRENT_BINARY_DIR}/Rag")
add_subdirectory (../Algorithms "${CMAKE_CURRENT_BINARY_DIR}/Algorithms")
add_subdirectory (../FeatureManager "${CMAKE_CURRENT_BINARY_DIR}/FeatureManager")
add_subdirectory (../IO "${CMAKE_CURRENT_BINARY_DIR}/IO")

add_executable (basic_rag_test Rag/basic_rag.cpp)
add_executable (basic_stack_test Stack/basic_stack.cpp)


if (NOT BUILDEM_DIR STREQUAL "None")
    add_dependencies (basic_rag_test ${hdf5_NAME} ${jsoncpp_NAME} ${boost_NAME})
    add_dependencies (basic_stack_test ${hdf5_NAME} ${jsoncpp_NAME} ${boost_NAME})
else()
    set (json_LIB jsoncpp)
    set (hdf5_LIBRARIES hdf5 hdf5_hl)
    set (boost_LIBS boost_thread boost_system boost_program_options boost_unit_test_framework boost_filesystem)
endif()

target_link_libraries (basic_rag_test Rag ${hdf5_libraries} ${json_LIB} ${boost_LIBS})
target_link_libraries (basic_stack_test Stack Rag ${hdf5_libraries} ${json_LIB} ${boost_LIBS} )

get_target_property (basic_rag_test_exe basic_rag_test LOCATION)
get_target_property (basic_stack_test_exe basic_stack_test LOCATION)


if (NOT ${CMAKE_SOURCE_DIR} STREQUAL ${BUILDLOC})  
    add_custom_command (
        TARGET basic_rag_test 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${basic_rag_test_exe} ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E remove ${basic_rag_test_exe})

    add_custom_command (
        TARGET basic_stack_test 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${basic_stack_test_exe} ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E remove ${basic_stack_test_exe})
endif()

add_test ("simple_rag_unit_tests" ${CMAKE_SOURCE_DIR}/bin/basic_rag_test)

add_test ("simple_stack_unit_tests"
        ${CMAKE_SOURCE_DIR}/bin/basic_stack_test
        ${CMAKE_SOURCE_DIR}/Stack/unit_tests/samp1_labels.h5
        ${CMAKE_SOURCE_DIR}/Stack/unit_tests/samp1_prediction.h5
        )
