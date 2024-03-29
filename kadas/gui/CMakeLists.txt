FILE(GLOB kadas_gui_SRC
  *.cpp
  catalog/*.cpp
  mapitems/*.cpp
  mapitemeditors/*.cpp
  maptools/*.cpp
  milx/*.cpp
  search/*.cpp
)
LIST(SORT kadas_gui_SRC)

FILE(GLOB kadas_gui_HDR
  *.h
  catalog/*.h
  mapitems/*.h
  mapitemeditors/*.h
  maptools/*.h
  milx/*.h
  search/*.h
)
LIST(SORT kadas_gui_HDR)

FILE(GLOB kadas_gui_UI ui/*.ui)
LIST(SORT kadas_gui_UI)

QT5_WRAP_UI(kadas_gui_UI_HDR ${kadas_gui_UI})

SET_PROPERTY(SOURCE ${kadas_gui_UI} PROPERTY SKIP_AUTOMOC ON)

ADD_LIBRARY(kadas_gui
    ${kadas_gui_SRC}
    ${kadas_gui_HDR}
    ${kadas_gui_UI}
)

pkg_search_module(CAIRO IMPORTED_TARGET cairo REQUIRED)
pkg_search_module(LIBRSVG IMPORTED_TARGET librsvg-2.0 REQUIRED)
pkg_search_module(SVG2SVGT IMPORTED_TARGET svg2svgt REQUIRED)
find_package(exiv2 REQUIRED)
find_package(Qwt REQUIRED)
find_package(GeographicLib REQUIRED)

TARGET_LINK_LIBRARIES(kadas_gui
  Qt5::Widgets
  Qt5::Network
  Qt5::Svg
  Qt5::Xml
  Qt5::Concurrent
  QuaZip::QuaZip
  QGIS::Gui
  ${GeographicLib_LIBRARIES}
  PkgConfig::LIBRSVG
  PkgConfig::SVG2SVGT
  PkgConfig::CAIRO
  Exiv2::exiv2lib
  Qwt::Qwt
  kadas_core
  kadas_analysis
)

target_include_directories(kadas_gui PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

GENERATE_EXPORT_HEADER(
  kadas_gui
  BASE_NAME KADAS_GUI
  EXPORT_FILE_NAME kadas_gui.h
)
SET(kadas_gui_HDR ${kadas_gui_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_gui.h)

INSTALL(TARGETS kadas_gui
  RUNTIME DESTINATION bin COMPONENT libraries
  ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT libraries
  LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT libraries)

INSTALL(FILES ${kadas_gui_HDR} DESTINATION include/kadas/gui)
