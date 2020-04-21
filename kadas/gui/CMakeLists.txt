SET(CMAKE_AUTOMOC ON)

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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${GeographicLib_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${QWT_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GDAL_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${EXIV2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CAIRO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${LIBRSVG_INCLUDE_DIRS})

ADD_LIBRARY(kadas_gui SHARED
    ${kadas_gui_SRC}
    ${kadas_gui_HDR}
    ${kadas_gui_UI}
)

TARGET_LINK_LIBRARIES(kadas_gui
  Qt5::Widgets
  Qt5::Network
  Qt5::Svg
  Qt5::Xml
  Qt5::Concurrent
  ${QGIS_CORE_LIBRARY}
  ${QGIS_GUI_LIBRARY}
  ${GeographicLib_LIBRARIES}
  ${QWT_LDFLAGS}
  ${GDAL_LDFLAGS}
  ${EXIV2_LDFLAGS}
  ${QUAZIP_LIBRARIES}
  ${CAIRO_LIBRARIES}
  ${LIBRSVG_LIBRARIES}
  kadas_core
  kadas_analysis
)

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
