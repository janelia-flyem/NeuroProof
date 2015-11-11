CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (IO)

set (SOURCES RagIO.cpp StackIO.cpp)

    if (APPLE) 
	add_library (IO ${SOURCES})
    else()
	add_library (IO SHARED ${SOURCES})
    endif()	

install (TARGETS IO DESTINATION lib${LIB_SUFFIX})
