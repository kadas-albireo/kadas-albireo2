SET(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Python 3.7 REQUIRED COMPONENTS Interpreter Development)

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

FILE(GLOB kadas_SRC
  *.cpp
  milx/*.cpp
  kml/*.cpp
  bullseye/*.cpp
  guidegrid/*.cpp
  iamauth/*.cpp
  mapgrid/*.cpp)
IF(WITH_GLOBE)
    FILE(GLOB globe_SRC globe/*.cpp globe/osg/*.cpp)
    SET(kadas_SRC ${kadas_SRC} ${globe_SRC})
ENDIF(WITH_GLOBE)
LIST(SORT kadas_SRC)

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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${QCA_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${QTKEYCHAIN_INCLUDE_DIRS}/qt5keychain)
INCLUDE_DIRECTORIES(${Python_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CAIRO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${LIBRSVG_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GEOS_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${GDAL_INCLUDE_DIR})

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
  Qt5::WebKitWidgets
  QuaZip::QuaZip
  ${AXCONTAINER_LIBRARY}
  ${QGIS_CORE_LIBRARY}
  ${QGIS_GUI_LIBRARY}
  ${CRASHRPT_LIBRARY}
  ${QCA_LDFLAGS}
  ${QTKEYCHAIN_LIBRARIES}
  ${Python_LIBRARIES}
  ${CAIRO_LIBRARIES}
  ${LIBRSVG_LIBRARIES}
  ${kadas_RC_LIBS}
  osgEarth

  kadas_core
  kadas_analysis
  kadas_gui
)

FILE(WRITE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kadassourcedir.txt ${PROJECT_SOURCE_DIR})

IF(WITH_GLOBE)
ADD_SUBDIRECTORY(globe/featuresource)
ENDIF(WITH_GLOBE)

INSTALL(TARGETS kadas DESTINATION bin)
INSTALL(FILES ${kadas_QM} DESTINATION share/kadas/locale)
