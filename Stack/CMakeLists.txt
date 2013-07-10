CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Stack)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")

set (SOURCES VolumeLabelData.cpp StackController.cpp)

include_directories (BEFORE ${FLYEM_BUILD_DIR}/include ${PYTHON_INCLUDE_PATH})


add_library (Stack SHARED ${SOURCES})
add_dependencies (Stack ${boost_NAME} ${vigra_NAME} ${hdf5_LIBRARIES})

# TODO: prevent default unit test build
# build unit test in main build by default
add_executable (basic_stack_test unit_tests/basic_stack.cpp VolumeLabelData.cpp)
add_dependencies (basic_stack_test ${boost_NAME} ${vigra_NAME} ${hdf5_NAMES})
target_link_libraries (basic_stack_test ${BUILDEM_LIB_DIR}/libboost_unit_test_framework.so ${hdf5_LIBRARIES} ${boost_LIBS} ${vigra_LIB} ${PYTHON_LIBRARY_FILE})

add_test ("simple_stack_unit_tests"
        ${BUILDEM_DIR}/bin/basic_stack_test
        ${CMAKE_SOURCE_DIR}/Stack/unit_tests/samp1_labels.h5
        ${CMAKE_SOURCE_DIR}/Stack/unit_tests/samp1_prediction.h5
        )

#add_executable (stack_test stack_test.cpp)
#add_dependencies (stack_test ${vigra_NAME} ${boost_NAME} ${hdf5_LIBRARIES})
#target_link_libraries (stack_test ${vigra_LIB} ${boost_LIBS} ${hdf5_LIBRARIES}  ${PYTHON_LIBRARY_FILE} Stack)

