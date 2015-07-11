CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (IO)

set (SOURCES RagIO.cpp StackIO.cpp)

add_library (IO ${SOURCES})

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    include (vigra)
    include (jsoncpp)
    include (hdf5)
    add_dependencies (Rag ${jsoncpp_NAME} ${boost_NAME} ${vigra_NAME}  ${hdf5_NAME})
endif()

install (TARGETS IO DESTINATION lib${LIB_SUFFIX})
