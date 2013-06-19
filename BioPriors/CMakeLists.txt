CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (BioPriors)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")

set (SOURCES BioStackController.cpp StackAgglomAlgs.cpp StackLearnAlgs.cpp)

include_directories (BEFORE ${FLYEM_BUILD_DIR}/include ${PYTHON_INCLUDE_PATH})

add_library (BioPriors SHARED ${SOURCES})
add_dependencies (BioPriors ${boost_NAME} ${vigra_NAME} ${hdf5_LIBRARIES})


#add_executable (stack_test stack_test.cpp)
#add_dependencies (stack_test ${vigra_NAME} ${boost_NAME} ${hdf5_LIBRARIES})
#target_link_libraries (stack_test ${vigra_LIB} ${boost_LIBS} ${hdf5_LIBRARIES}  ${PYTHON_LIBRARY_FILE} Stack)

