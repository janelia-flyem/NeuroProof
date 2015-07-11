CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Stack)

set (SOURCES VolumeLabelData.cpp Stack.cpp )
add_library (Stack ${SOURCES})

# prevent vigra from overwriting boost declarations
set_target_properties(Stack PROPERTIES COMPILE_DEFINITIONS "WITH_BOOST_GRAPH")

# include vigra directories if none are built or included in the system path
include_directories (AFTER ${MAKE_CURRENT_BINARY_DIR}/src/external_packages)

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    add_dependencies (Stack ${boost_NAME})
endif()

install (TARGETS Stack DESTINATION lib${LIB_SUFFIX})
