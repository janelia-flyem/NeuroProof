CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Stack)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")


if (NOT DISABLE_GUI)
    set (SOURCES VolumeLabelData.cpp Stack.cpp StackSession.cpp StackPlaneController.cpp StackPlaneView.cpp StackBodyView.cpp StackBodyController.cpp)
else()
    set (SOURCES VolumeLabelData.cpp Stack.cpp StackSession.cpp )
endif()

if (QT_BUILT)
    QT4_WRAP_CPP(QTHEADERS StackPlaneController.h)
    add_library (Stack SHARED ${SOURCES} ${QTHEADERS})
else()
    add_library (Stack SHARED ${SOURCES})
endif()

if (NOT DISABLE_GUI)
    add_dependencies (Stack ${boost_NAME} ${vigra_NAME} ${hdf5_LIBRARIES} ${jsoncpp_NAME} ${opencv_NAME} RescanQt4)
else()
    add_dependencies (Stack ${boost_NAME} ${vigra_NAME} ${hdf5_LIBRARIES} ${jsoncpp_NAME} ${opencv_NAME})
endif()

add_executable (basic_stack_test unit_tests/basic_stack.cpp VolumeLabelData.cpp)

if (NOT BUILDEM_DIR STREQUAL "None")
    add_dependencies (basic_stack_test ${boost_NAME} ${vigra_NAME} ${hdf5_NAMES} RescanQt4)
endif()

target_link_libraries (basic_stack_test ${hdf5_LIBRARIES} ${boost_LIBS} ${vigra_LIB} ${PYTHON_LIBRARY_FILE})

get_target_property (basic_stack_test_exe basic_stack_test LOCATION)

if (NOT ${CMAKE_SOURCE_DIR} STREQUAL ${BUILDLOC})  
    add_custom_command (
        TARGET basic_stack_test 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${basic_stack_test_exe} ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E remove ${basic_stack_test_exe})
endif()

add_test ("simple_stack_unit_tests"
        ${CMAKE_SOURCE_DIR}/bin/basic_stack_test
        ${CMAKE_SOURCE_DIR}/Stack/unit_tests/samp1_labels.h5
        ${CMAKE_SOURCE_DIR}/Stack/unit_tests/samp1_prediction.h5
        )
