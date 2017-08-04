CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project (NeuroProof)

include (ExternalProject)

enable_testing()

set (RUN_ENVIRONMENT "Workstation" CACHE TYPE STRING)
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif ()

SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")

if (NOT APPLE)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS}")
endif()

set (CMAKE_CXX_LINK_FLAGS "-O3")
set (CMAKE_DEBUG_POSTFIX "-g")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
message("Using cmake modules from ${CMAKE_MODULE_PATH}")
set (BUILDLOC ${CMAKE_SOURCE_DIR})

message("making the python output dir...")
set(NEUROPROOF_PYTHON_BUILD_OUTPUT_DIR "${CMAKE_BINARY_DIR}/python")
make_directory( ${NEUROPROOF_PYTHON_BUILD_OUTPUT_DIR})

# Copy pure python source from source-dir to build-dir
file(COPY "${CMAKE_SOURCE_DIR}/python/neuroproof" DESTINATION "${NEUROPROOF_PYTHON_BUILD_OUTPUT_DIR}")

set(CTEST_ENVIRONMENT "PYTHONPATH=${NEUROPROOF_PYTHON_BUILD_OUTPUT_DIR}")

######################################################################
#
#      find default install directory for Python modules
#      (usually PYTHONDIR/Lib/site-packages)
#
######################################################################
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
    "from distutils.sysconfig import *; print(get_python_lib(1))"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
FILE(TO_CMAKE_PATH ${PYTHON_SITE_PACKAGES} NEUROPROOF_PYTHON_INSTALL_DIR)
SET(NEUROPROOF_PYTHON_INSTALL_DIR ${NEUROPROOF_PYTHON_INSTALL_DIR}
    CACHE PATH "where to install the NeuroProof Python package" FORCE)
# this is the install path relative to CMAKE_INSTALL_PREFIX,
# use this in INSTALL() commands to get packaging right
FILE(RELATIVE_PATH NEUROPROOF_PYTHON_INSTALL_DIR ${CMAKE_INSTALL_PREFIX} ${NEUROPROOF_PYTHON_INSTALL_DIR})

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILDLOC}/bin)
if (NOT EXISTS ${BUILDLOC}/bin)
    file (MAKE_DIRECTORY ${BUILDLOC}/bin)
endif() 

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILDLOC}/lib)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILDLOC}/lib)
if (NOT EXISTS ${BUILDLOC}/lib)
    file (MAKE_DIRECTORY ${BUILDLOC}/lib)
endif() 

include_directories (BEFORE ${CMAKE_SOURCE_DIR}/src)


# default gui enable off
set (ENABLE_GUI NO CACHE BOOL "Build GUI for NeuroProof")

FIND_PACKAGE(PythonLibs)
FIND_PACKAGE(Boost)
find_package(LIBDVIDCPP)


if (ENABLE_GUI)
    SET (QT_BUILT TRUE)
    FIND_PACKAGE(VTK REQUIRED)
    FIND_PACKAGE(Qt4 REQUIRED)

    INCLUDE(${VTK_USE_FILE})
    IF(NOT VTK_USE_RENDERING)
        MESSAGE(FATAL_ERROR
            "Example requires VTK_USE_RENDERING.")
    ENDIF(NOT VTK_USE_RENDERING)
    INCLUDE(${QT_USE_FILE})
    INCLUDE_DIRECTORIES(
        ${QT_INCLUDE_DIR}
        ${QT_QTGUI_INCLUDE_DIR}
        ${QT_QTCORE_INCLUDE_DIR}
    )
else()
    SET (QT_BUILT FALSE)
endif()

# ensure the libjsoncpp.so is symbolically linked somewhere your lib path
set (json_LIB jsoncpp)
set (hdf5_LIBRARIES hdf5 hdf5_hl)
set (vigra_LIB vigraimpex)
set (opencv_LIBS opencv_ml opencv_core)
set (boost_LIBS boost_thread boost_system boost_program_options boost_python boost_unit_test_framework boost_filesystem)
set (libdvid_LIBS ${LIBDVIDCPP_LIBRARY})

if (ENABLE_GUI)
    set (vtk_LIBS vtkHybrid vtkRendering vtkVolumeRendering vtkWidgets vtkCommon QVTK)
    set (qt_LIBS  ${QT_LIBRARIES})
endif()

set (PYTHON_LIBRARY_FILE ${PYTHON_LIBRARIES})
set (PYTHON_EXE "python") 

if (ENABLE_GUI)
    set (NEUROPROOF_INT_LIBS Rag Stack EdgeEditor Gui BioPriors FeatureManager
        Algorithms Classifier SemiSupervised StackGui IO)
    set (NEUROPROOF_EXT_LIBS ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} ${boost_LIBS} ${libdvid_LIBS} ${PYTHON_LIBRARY_FILE} ${vtk_LIBS} ${qt_LIBS})
else()
    set (NEUROPROOF_INT_LIBS Rag Stack EdgeEditor BioPriors IO FeatureManager Algorithms Classifier SemiSupervised)
    set (NEUROPROOF_EXT_LIBS ${json_LIB} ${hdf5_LIBRARIES} ${vigra_LIB} ${opencv_LIBS} ${boost_LIBS} ${libdvid_LIBS} ${PYTHON_LIBRARY_FILE})
endif()


include_directories (${Boost_INCLUDE_DIR})
include_directories (BEFORE ${PYTHON_INCLUDE_PATH})
include_directories(${LIBDVIDCPP_INCLUDE_DIRS})
include_directories (AFTER ${CMAKE_SOURCE_DIR}/src/external_packages)

# add the source executables
add_subdirectory(neuroproof)


# np packages to include that are shared across packages
add_subdirectory (src/Utilities)
add_subdirectory (src/Rag)
add_subdirectory (src/Stack)
add_subdirectory (src/IO)
add_subdirectory (src/EdgeEditor)
add_subdirectory (src/BioPriors)
add_subdirectory (src/FeatureManager)
add_subdirectory (src/Algorithms)
add_subdirectory (src/Classifier)
add_subdirectory (src/SemiSupervised)
add_subdirectory(unit_tests)
add_subdirectory(libMetrics)


# add python library
add_subdirectory(python)

