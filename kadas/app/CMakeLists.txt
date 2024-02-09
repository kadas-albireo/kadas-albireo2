FIND_PACKAGE(Python3 3.7 REQUIRED COMPONENTS Interpreter Development)

IF(WITH_GLOBE)    
    FIND_PACKAGE(OSG REQUIRED)
    FIND_PACKAGE(osgEarth REQUIRED)
    ADD_DEFINITIONS(-DWITH_GLOBE)
    INCLUDE_DIRECTORIES(${OSG_INCLUDE_DIR})
    
    SET(GLOBE_LIBS 
    ${OSGDB_LIBRARY}
    ${OSGGA_LIBRARY}
    ${OSGUTIL_LIBRARY}
    ${OSG_LIBRARY}
    ${OSGVIEWER_LIBRARY}
    ${OPENTHREADS_LIBRARY}
    )
ENDIF()

set(kadas_SRC
  kadasapplayerhandling.cpp
  kadasapplication.cpp
  kadascanvascontextmenu.cpp
  kadasgpsintegration.cpp
  kadasgpxintegration.cpp
  kadashandlebadlayers.cpp
  kadaslayerrefreshmanager.cpp
  kadaslayertreemodel.cpp
  kadaslayertreeviewmenuprovider.cpp
  kadaslayoutappmenuprovider.cpp
  kadaslayoutdesignerdialog.cpp
  kadaslayoutdesignermanager.cpp
  kadasmainwindow.cpp
  kadasmapidentifydialog.cpp
  kadasmapwidgetmanager.cpp
  kadasmessagelogviewer.cpp
  kadasnewspopup.cpp
  kadasplugininterfaceimpl.cpp
  kadaspluginlayerproperties.cpp
  kadaspluginmanager.cpp
  kadaspythonintegration.cpp
  kadasredliningintegration.cpp
  main.cpp
  bullseye/kadasbullseyelayer.cpp
  bullseye/kadasmaptoolbullseye.cpp
  guidegrid/kadasguidegridlayer.cpp
  guidegrid/kadasmaptoolguidegrid.cpp
  iamauth/kadasiamauth.cpp
  kml/kadaskmlexport.cpp
  kml/kadaskmlexportdialog.cpp
  kml/kadaskmlimport.cpp
  kml/kadaskmlintegration.cpp
  kml/kadaskmllabeling.cpp
  mapgrid/kadasmapgridlayer.cpp
  mapgrid/kadasmaptoolmapgrid.cpp
  milx/kadasmilxintegration.cpp
)

set(WITH_CRASHREPORT OFF CACHE BOOL "Enable crash report")

if(WITH_CRASHREPORT)
  set(kadas_SRC ${kadas_SRC} kadascrashrpt.cpp)
endif()

if(WITH_GLOBE)
  set(kadas_SRC ${kadas_SRC}
    globe/kadasglobebillboardmanager.cpp
    globe/kadasglobedialog.cpp
    globe/kadasglobeintegration.cpp
    globe/kadasglobeinteractionhandlers.cpp
    globe/kadasglobeprojectlayermanager.cpp
    globe/kadasglobetilesource.cpp
    globe/kadasglobevectorlayerproperties.cpp
    globe/kadasglobewidget.cpp
    globe/osg/osgQtGraphicsWindow.cpp
    globe/osg/osgQtViewerWidget.cpp
  )
endif(WITH_GLOBE)

FILE(GLOB kadas_HDR
  *.h
  milx/*.h
  kml/*.h
  bullseye/*.h
  guidegrid/*.h
  iamauth/*.h
  mapgrid/*.h)
IF(WITH_GLOBE)
    FILE(GLOB globe_HDR globe/*.h globe/osg/*.h)
    SET(kadas_HDR ${kadas_HDR} ${globe_HDR})
ENDIF(WITH_GLOBE)
LIST(SORT kadas_HDR)

FILE(GLOB kadas_UI
  ui/*.ui
  kml/*.ui
  bullseye/*.ui
  guidegrid/*.ui
  mapgrid/*.ui)
IF(WITH_GLOBE)
    FILE(GLOB globe_UI globe/*.ui)
    SET(kadas_UI ${kadas_UI} ${globe_UI})
ENDIF(WITH_GLOBE)
LIST(SORT kadas_UI)

SET(KADAS_SPLASH "splash.png" CACHE STRING "Splash image")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/kadas/resources/resources.qrc.in ${CMAKE_SOURCE_DIR}/kadas/resources/resources.qrc @ONLY)
SET(kadas_RCC_SRC ${CMAKE_SOURCE_DIR}/kadas/resources/resources.qrc)

FILE(GLOB kadas_TS ${CMAKE_SOURCE_DIR}/locale/*.ts)
LIST(SORT kadas_TS)
SET_SOURCE_FILES_PROPERTIES(${kadas_TS} PROPERTIES OUTPUT_LOCATION ${KADAS_OUTPUT_DIRECTORY}/locale)


QT5_ADD_RESOURCES(kadas_RCC ${kadas_RCC_SRC})
QT5_WRAP_UI(kadas_UI_HDR ${kadas_UI})
QT5_ADD_TRANSLATION(kadas_QM ${kadas_TS})

IF(MINGW)
  ENABLE_LANGUAGE(RC)
  SET(CMAKE_RC_COMPILER_INIT ${CMAKE_GENERATOR_RC})
  SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -i <SOURCE> -o <OBJECT>")
  SET(kadas_SRC ${kadas_SRC} ../resources/kadas-icon.rc)
  SET(kadas_RC_LIBS -mwindows)
ENDIF(MINGW)

SET_PROPERTY(SOURCE ${kadas_UI_HDR} PROPERTY SKIP_AUTOMOC ON)
SET_PROPERTY(SOURCE ${kadas_RCC} PROPERTY SKIP_AUTOMOC ON)

IF(WIN32)
  FIND_PACKAGE(Qt5AxContainer REQUIRED)
  SET(AXCONTAINER_LIBRARY Qt5::AxContainer)
ENDIF(WIN32)


ADD_EXECUTABLE(kadas
    ${kadas_SRC}
    ${kadas_HDR}
    ${kadas_UI}
    ${kadas_UI_HDR}
    ${kadas_RCC_SRC}
    ${kadas_RCC}
    ${kadas_QM}
)

TARGET_LINK_LIBRARIES(kadas
  Qt5::Widgets
  Qt5::Network
  Qt5::Xml
  Qt5::Sql
  Qt5::OpenGL
  Qt5::PrintSupport
  QuaZip::QuaZip
  ${AXCONTAINER_LIBRARY}
  QGIS::Core
  QGIS::Gui
  Python3::Interpreter
  ${kadas_RC_LIBS}

  kadas_core
  kadas_analysis
  kadas_gui
)

target_include_directories(kadas PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(WITH_CRASHREPORT)
  target_link_libraries(kadas ${CRASHRPT_LIBRARY})
  target_compile_definitions(kadas PRIVATE WITH_CRASHREPORT)
endif()

if (WITH_QTWEBKIT)
  TARGET_LINK_LIBRARIES(kadas Qt5::WebKitWidgets)
endif()

if (WITH_GLOBE)
  TARGET_LINK_LIBRARIES(kadas osgEarth)
  ADD_SUBDIRECTORY(globe/featuresource)
endif()

if (WITH_BINDINGS)
 add_definitions(-DWITH_BINDINGS)
endif()

if (WITH_QTWEBKIT)
 add_definitions(-DWITH_QTWEBKIT)
endif()

FILE(WRITE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kadassourcedir.txt ${PROJECT_SOURCE_DIR})

INSTALL(TARGETS kadas DESTINATION bin)
INSTALL(FILES ${kadas_QM} DESTINATION share/kadas/locale)
