file(
  GLOB
  kadas_gui_SRC
  *.cpp
  catalog/*.cpp
  mapitems/*.cpp
  mapitemeditors/*.cpp
  maptools/*.cpp
  milx/*.cpp
  search/*.cpp)
list(SORT kadas_gui_SRC)

file(
  GLOB
  kadas_gui_HDR
  *.h
  catalog/*.h
  mapitems/*.h
  mapitemeditors/*.h
  maptools/*.h
  milx/*.h
  search/*.h)
list(SORT kadas_gui_HDR)

file(GLOB kadas_gui_UI ui/*.ui)
list(SORT kadas_gui_UI)

qt5_wrap_ui(kadas_gui_UI_HDR ${kadas_gui_UI})

set_property(SOURCE ${kadas_gui_UI} PROPERTY SKIP_AUTOMOC ON)

add_library(kadas_gui ${kadas_gui_SRC} ${kadas_gui_HDR} ${kadas_gui_UI})

pkg_search_module(CAIRO IMPORTED_TARGET cairo REQUIRED)
pkg_search_module(LIBRSVG IMPORTED_TARGET librsvg-2.0 REQUIRED)
pkg_search_module(SVG2SVGT IMPORTED_TARGET svg2svgt REQUIRED)

find_package(Qwt REQUIRED)
find_package(GeographicLib REQUIRED)
find_package(exiv2 REQUIRED)

target_link_libraries(
  kadas_gui
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
  kadas_analysis)

target_include_directories(kadas_gui PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

generate_export_header(kadas_gui BASE_NAME KADAS_GUI EXPORT_FILE_NAME
                       kadas_gui.h)
set(kadas_gui_HDR ${kadas_gui_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_gui.h)

install(
  TARGETS kadas_gui
  RUNTIME DESTINATION bin COMPONENT libraries
  ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT libraries
  LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT libraries)

install(FILES ${kadas_gui_HDR} DESTINATION include/kadas/gui)
