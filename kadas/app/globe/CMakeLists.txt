# set path to additional CMake modules
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH})

FIND_PACKAGE(OSG REQUIRED)
FIND_PACKAGE(osgearth REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)

########################################################
# Files

SET (GLOBE_PLUGIN_SRCS
    kadasglobeintegration.cpp
    kadasglobetilesource.cpp
    kadasglobedialog.cpp
    kadasglobevectorlayerproperties.cpp
    kadasglobefeatureidentify.cpp
    kadasglobefrustumhighlight.cpp
    kadasglobewidget.cpp
)

SET (GLOBE_PLUGIN_UIS
    kadasglobedialog.ui
    kadasglobevectorlayerpropertiespage.ui
)

SET (GLOBE_PLUGIN_MOC_HDRS
    kadasglobeintegration.h
    kadasglobedialog.h
    kadasglobevectorlayerproperties.h
    kadasglobetilesource.h
    kadasglobewidget.h
)

SET (GLOBE_PLUGIN_HDRS
    kadasglobevectorlayerproperties.h
    kadasglobefeatureidentify.h
    kadasglobefrustumhighlight.h
)

SET (GLOBE_PLUGIN_RCCS kadasglobe.qrc)

########################################################
# Build

IF(WIN32)
    ADD_DEFINITIONS(-DGLOBE_EXPORT=${DLLEXPORT})
ELSE(WIN32)
    ADD_DEFINITIONS(-DGLOBE_EXPORT=)
ENDIF(WIN32)

QT5_WRAP_UI (GLOBE_PLUGIN_UIS_H  ${GLOBE_PLUGIN_UIS})

QT5_WRAP_CPP (GLOBE_PLUGIN_MOC_SRCS  ${GLOBE_PLUGIN_MOC_HDRS})

QT5_ADD_RESOURCES(GLOBE_PLUGIN_RCC_SRCS ${GLOBE_PLUGIN_RCCS})

ADD_LIBRARY (globeplugin MODULE ${GLOBE_PLUGIN_SRCS} ${GLOBE_PLUGIN_MOC_SRCS} ${GLOBE_PLUGIN_RCC_SRCS} ${GLOBE_PLUGIN_UIS_H} ${GLOBE_PLUGIN_HDRS})
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Qt5OpenGL_INCLUDE_DIRS}
  ${OSG_INCLUDE_DIR}
  ${SIP_INCLUDE_DIR}
  ../../core
  ../../core/expression
  ../../core/geometry
  ../../core/metadata
  ../../core/raster
  ../../gui
  ..
  ${CMAKE_BINARY_DIR}/src/core
  ${CMAKE_BINARY_DIR}/src/gui
  ${CMAKE_SOURCE_DIR}/external
)

TARGET_LINK_LIBRARIES(globeplugin
  qgis_core
  qgis_gui
  Qt5::OpenGL
  ${OSGDB_LIBRARY}
  ${OSGGA_LIBRARY}
  ${OSGUTIL_LIBRARY}
  ${OSG_LIBRARY}
  ${OSGQT_LIBRARY}
  ${OSGVIEWER_LIBRARY}
  osgEarth
  ${OPENTHREADS_LIBRARY}
)

ADD_SUBDIRECTORY(featuresource)

########################################################
# Install

INSTALL(TARGETS globeplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

INSTALL (FILES images/world.tif
         DESTINATION ${QGIS_DATA_DIR}/globe)

INSTALL (DIRECTORY images/gui
         DESTINATION ${QGIS_DATA_DIR}/globe)
