CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Rag)

set (SOURCES RagUtils.cpp)
    if (APPLE) 
	add_library (Rag ${SOURCES})
    else()
	add_library (Rag SHARED ${SOURCES})
    endif()	

install (TARGETS Rag DESTINATION lib${LIB_SUFFIX})
