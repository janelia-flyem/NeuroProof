CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (SemiSupervised)

set (SOURCES kmeans.cpp weightmatrix1.cpp)

include_directories (AFTER ${CMAKE_SOURCE_DIR}/src/external_packages)

    if (APPLE) 
	add_library (SemiSupervised ${SOURCES})
    else()
	add_library (SemiSupervised SHARED ${SOURCES})
    endif()	




if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    include (vigra)
    add_dependencies (SemiSupervised ${boost_NAME} ${vigra_NAME} ${opencv_NAME})
endif()

install (TARGETS SemiSupervised DESTINATION lib${LIB_SUFFIX})

