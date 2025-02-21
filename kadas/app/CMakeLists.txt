find_package(Python3 3.7 REQUIRED COMPONENTS Interpreter Development)

set(kadas_SRC
    main.cpp
    kadasapplayerhandling.cpp
    kadasapplication.cpp
    kadasbookmarksmenu.cpp
    kadascanvascontextmenu.cpp
    kadasfeedback.cpp
    kadasgpsintegration.cpp
    kadasgpxintegration.cpp
    kadashandlebadlayers.cpp
    kadashelpviewer.cpp
    kadaslayerrefreshmanager.cpp
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
    kadastemporalcontroller.cpp
    3d/kadas3dintegration.cpp
    3d/kadas3dlayertreemodel.cpp
    3d/kadas3dmapcanvaswidget.cpp
    3d/kadas3dmapconfigwidget.cpp
    3d/kadas3dnavigationwidget.cpp
    3d/kadasambientocclusionsettingswidget.cpp
    3d/kadaslightswidget.cpp
    3d/kadasmesh3dsymbolwidget.cpp
    3d/kadasphongmaterialwidget.cpp
    3d/kadasshadowrenderingsettingswidget.cpp
    3d/kadasskyboxrenderingsettingswidget.cpp
    bullseye/kadasbullseyelayer.cpp
    bullseye/kadasmaptoolbullseye.cpp
    external/qgis/app/qgslayertreeviewindicatorprovider.cpp
    guidegrid/kadasguidegridlayer.cpp
    guidegrid/kadasmaptoolguidegrid.cpp
    iamauth/kadasiamauth.cpp
    kml/kadaskmlexport.cpp
    kml/kadaskmlexportdialog.cpp
    kml/kadaskmlimport.cpp
    kml/kadaskmlintegration.cpp
    kml/kadaskmllabeling.cpp
    layertree/kadaslayertreeviewmenuprovider.cpp
    layertree/kadaslayertreeviewtemporalindicator.cpp
    mapgrid/kadasmapgridlayer.cpp
    mapgrid/kadasmapgridlayerrenderer.cpp
    mapgrid/kadasmaptoolmapgrid.cpp
    mapswipe/kadasmapswipecanvasitem.cpp
    mapswipe/kadasmapswipetool.cpp
    milx/kadasmilxexportdialog.cpp
    milx/kadasmilxintegration.cpp
)

set(kadas_HDR
    kadasapplayerhandling.h
    kadasapplication.h
    kadasbookmarksmenu.h
    kadascanvascontextmenu.h
    kadasfeedback.h
    kadasgpsintegration.h
    kadasgpxintegration.h
    kadashandlebadlayers.h
    kadashelpviewer.h
    kadaslayerrefreshmanager.h
    kadaslayoutappmenuprovider.h
    kadaslayoutdesignerdialog.h
    kadaslayoutdesignermanager.h
    kadasmainwindow.h
    kadasmapidentifydialog.h
    kadasmapwidgetmanager.h
    kadasmessagelogviewer.h
    kadasnewspopup.h
    kadasplugininterfaceimpl.h
    kadaspluginlayerproperties.h
    kadaspluginmanager.h
    kadaspythonintegration.h
    kadasredliningintegration.h
    kadastemporalcontroller.h
    3d/kadas3dintegration.h
    3d/kadas3dlayertreemodel.h
    3d/kadas3dmapcanvaswidget.h
    3d/kadas3dmapconfigwidget.h
    3d/kadas3dnavigationwidget.h
    3d/kadasambientocclusionsettingswidget.h
    3d/kadaslightswidget.h
    3d/kadasmesh3dsymbolwidget.h
    3d/kadasphongmaterialwidget.h
    3d/kadasshadowrenderingsettingswidget.h
    3d/kadasskyboxrenderingsettingswidget.h
    bullseye/kadasbullseyelayer.h
    bullseye/kadasmaptoolbullseye.h
    external/qgis/app/qgslayertreeviewindicatorprovider.h
    guidegrid/kadasguidegridlayer.h
    guidegrid/kadasmaptoolguidegrid.h
    iamauth/kadasiamauth.h
    kml/kadaskmlexport.h
    kml/kadaskmlexportdialog.h
    kml/kadaskmlimport.h
    kml/kadaskmlintegration.h
    kml/kadaskmllabeling.h
    layertree/kadaslayertreeviewmenuprovider.h
    layertree/kadaslayertreeviewtemporalindicator.h
    mapgrid/kadasmapgridlayer.h
    mapgrid/kadasmapgridlayerrenderer.h
    mapgrid/kadasmaptoolmapgrid.h
    mapswipe/kadasmapswipecanvasitem.h
    mapswipe/kadasmapswipetool.h
    milx/kadasmilxexportdialog.h
    milx/kadasmilxintegration.h
)

set(WITH_CRASHREPORT
    OFF
    CACHE BOOL "Enable crash report"
)

if(WITH_CRASHREPORT)
  set(kadas_SRC ${kadas_SRC} kadascrashrpt.cpp)
endif()

list(SORT kadas_HDR)

file(
  GLOB
  kadas_UI
  ui/*.ui
  kml/*.ui
  bullseye/*.ui
  guidegrid/*.ui
  mapgrid/*.ui
  milx/*.ui
)
list(SORT kadas_UI)

set(KADAS_SPLASH
    "splash.png"
    CACHE STRING "Splash image"
)
configure_file(
  ${CMAKE_SOURCE_DIR}/kadas/resources/resources.qrc.in
  ${CMAKE_SOURCE_DIR}/kadas/resources/resources.qrc @ONLY
)
set(KADAS_RCC_SRC ${CMAKE_SOURCE_DIR}/kadas/resources/resources.qrc)

# Translation
find_package(Qt5 COMPONENTS LinguistTools)
file(GLOB TS_FILES ${CMAKE_SOURCE_DIR}/i18n/*.ts)
set_source_files_properties(
  ${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/i18n
)
qt_add_translation(QM_FILES ${TS_FILES})

set(KADAS_QM "${CMAKE_BINARY_DIR}/translations.qrc")
file(WRITE ${KADAS_QM}
     "<!DOCTYPE RCC><RCC version=\"1.0\">\n  <qresource prefix=\"/i18n/\">\n"
)
foreach(QM_FILE ${QM_FILES})
  get_filename_component(QM_FILE_NAME ${QM_FILE} NAME)
  file(APPEND ${KADAS_QM}
       "\n        <file alias=\"${QM_FILE_NAME}\">${QM_FILE}</file>"
  )
endforeach()
file(APPEND ${KADAS_QM} "\n  </qresource>\n</RCC>\n")

qt5_add_resources(KADAS_RCC ${KADAS_QM})

qt5_add_resources(KADAS_RCC ${KADAS_RCC_SRC})
qt5_wrap_ui(kadas_UI_HDR ${kadas_UI})

if(WIN32)
  enable_language(RC)
  list(APPEND kadas_SRC "${CMAKE_SOURCE_DIR}/kadas/resources/kadas-icon.rc")
endif()
if(MINGW)
  set(CMAKE_RC_COMPILER_INIT ${CMAKE_GENERATOR_RC})
  set(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> -O coff -i <SOURCE> -o <OBJECT>"
  )
  set(kadas_RC_LIBS -mwindows)
endif(MINGW)

set_property(SOURCE ${kadas_UI_HDR} PROPERTY SKIP_AUTOMOC ON)
set_property(SOURCE ${KADAS_RCC} PROPERTY SKIP_AUTOMOC ON)

if(WIN32)
  find_package(Qt5AxContainer REQUIRED)
  set(AXCONTAINER_LIBRARY Qt5::AxContainer)

  set(SHOW_CONSOLE
      OFF
      CACHE BOOL "Show console (Windows)"
  )
  if(NOT SHOW_CONSOLE)
    set(WIN32_SHOW_CONSOLE "WIN32")
  endif()
endif(WIN32)

add_executable(
  kadas
  ${WIN32_SHOW_CONSOLE}
  ${kadas_SRC}
  ${kadas_HDR}
  ${kadas_UI}
  ${kadas_UI_HDR}
  ${KADAS_RCC_SRC}
  ${KADAS_RCC}
  ${kadas_QM}
)

target_link_libraries(
  kadas
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
  QGIS::3D
  Python3::Python
  ${kadas_RC_LIBS}
  kadas_core
  kadas_analysis
  kadas_gui
)

target_include_directories(
  kadas
  PUBLIC ${CMAKE_SOURCE_DIR}/kadas/app
         ${CMAKE_SOURCE_DIR}/kadas/app/3d
         ${CMAKE_SOURCE_DIR}/kadas/app/bullseye
         ${CMAKE_SOURCE_DIR}/kadas/app/external/qgis
         ${CMAKE_SOURCE_DIR}/kadas/app/guidegrid
         ${CMAKE_SOURCE_DIR}/kadas/app/iamauth
         ${CMAKE_SOURCE_DIR}/kadas/app/kml
         ${CMAKE_SOURCE_DIR}/kadas/app/layertree
         ${CMAKE_SOURCE_DIR}/kadas/app/mapgrid
         ${CMAKE_SOURCE_DIR}/kadas/app/mapswipe
         ${CMAKE_SOURCE_DIR}/kadas/app/milx
         ${CMAKE_BINARY_DIR}/kadas/app # UI are compiled there
)

if(WITH_VCPKG)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kadas.env.in
    ${CMAKE_CURRENT_BINARY_DIR}/kadas.env @ONLY
  )
endif()

if(WITH_CRASHREPORT)
  target_link_libraries(kadas ${CRASHRPT_LIBRARY})
  target_compile_definitions(kadas PRIVATE WITH_CRASHREPORT)
endif()

if(WITH_QTWEBKIT)
  target_link_libraries(kadas Qt5::WebKitWidgets)
endif()

if(WITH_BINDINGS)
  add_definitions(-DWITH_BINDINGS)
endif()

if(WITH_QTWEBKIT)
  add_definitions(-DWITH_QTWEBKIT)
endif()

file(WRITE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kadassourcedir.txt
     ${PROJECT_SOURCE_DIR}
)

install(TARGETS kadas DESTINATION bin)
install(FILES ${KADAS_QM} DESTINATION share/kadas/i18n)
