CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (BioPriors)

set (SOURCES BioStack.cpp StackAgglomAlgs.cpp StackLearnAlgs.cpp IterativeLearn.cpp Dataset.cpp IterativeLearn_semi.cpp IterativeLearn_unc.cpp IterativeLearn_iwal.cpp IterativeLearn_cotrain.cpp IterativeLearn_simulate.cpp)

include_directories (AFTER ${CMAKE_SOURCE_DIR}/src/external_packages)

add_library (BioPriors SHARED ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    include (vigra)
    include (opencv)
    add_dependencies (BioPriors ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
endif()


