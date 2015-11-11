CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (FeatureManager)

set (SOURCES FeatureMgr.cpp Features.cpp)

if (APPLE) 
	add_library (FeatureManager ${SOURCES})
else()
	add_library (FeatureManager SHARED ${SOURCES})
endif()	

install (TARGETS FeatureManager DESTINATION lib${LIB_SUFFIX})
