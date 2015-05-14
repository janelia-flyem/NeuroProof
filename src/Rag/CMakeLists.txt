CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Rag)

set (SOURCES RagUtils.cpp)
add_library (Rag SHARED ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    add_dependencies (Rag ${boost_NAME})
endif()


