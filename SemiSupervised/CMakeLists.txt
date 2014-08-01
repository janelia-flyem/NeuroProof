CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (SemiSupervised)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -w")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb -w")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")


set (SOURCES kmeans.cpp weightmatrix1.cpp)

add_library (SemiSupervised SHARED ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    add_dependencies (SemiSupervised ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
endif()

#add_executable (stack_test stack_test.cpp)
#add_dependencies (stack_test ${vigra_NAME} ${boost_NAME} ${hdf5_LIBRARIES})
#target_link_libraries (stack_test ${vigra_LIB} ${boost_LIBS} ${hdf5_LIBRARIES}  ${PYTHON_LIBRARY_FILE} Stack)

