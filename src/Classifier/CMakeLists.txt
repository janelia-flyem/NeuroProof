CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Classifier)

set (SOURCES opencvABclassifier.cpp opencvRFclassifier.cpp opencvSVMclassifier.cpp vigraRFclassifier.cpp)

if (APPLE) 
	add_library (Classifier ${SOURCES})
else()
	add_library (Classifier SHARED ${SOURCES})
endif()	

if (NOT BUILDEM_DIR STREQUAL "None")
    include (boost)
    include (vigra)
    include (opencv)
    add_dependencies (Classifier ${vigra_NAME} ${opencv_NAME} ${boost_NAME})
endif()

install (TARGETS Classifier DESTINATION lib${LIB_SUFFIX})
