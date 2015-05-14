CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (GUI)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

set (SOURCES StackQTController.cpp StackQTUi.cpp)

if (QT_BUILT)
    QT4_WRAP_CPP(QTHEADERS StackQTUi.h StackQTController.h)
    QT4_WRAP_UI(QTFORMS neuroproof_stack_viewer.ui)
    add_library (Gui SHARED ${SOURCES} ${QTFORMS} ${QTHEADERS})
else()
    add_library (Gui SHARED ${SOURCES})
endif()


if (NOT BUILDEM_DIR STREQUAL "None")
    include (qt4)
    include (vtk)
    include (boost) 
    add_dependencies (Gui ${boost_NAME} ${vtk_NAME} ${qt4_NAME} RescanQt4)
else()
    add_dependencies (Gui RescanQt4)
endif()
