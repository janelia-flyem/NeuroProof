CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Stack)

set (SOURCES VolumeLabelData.cpp Stack.cpp )
    if (APPLE) 
	add_library (Stack ${SOURCES})
    else()
	add_library (Stack SHARED ${SOURCES})
    endif()	


# prevent vigra from overwriting boost declarations
set_target_properties(Stack PROPERTIES COMPILE_DEFINITIONS "WITH_BOOST_GRAPH")

# include vigra directories if none are built or included in the system path
include_directories (AFTER ${MAKE_CURRENT_BINARY_DIR}/src/external_packages)

install (TARGETS Stack DESTINATION lib${LIB_SUFFIX})
