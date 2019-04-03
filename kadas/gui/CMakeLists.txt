FILE(GLOB kadas_gui_SRC *.cpp)
LIST(SORT kadas_gui_SRC)

FILE(GLOB kadas_gui_HDR *.h)
LIST(SORT kadas_gui_HDR)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${GeographicLib_INCLUDE_DIR})

ADD_LIBRARY(kadas_gui SHARED
    ${kadas_gui_SRC}
    ${kadas_gui_HDR}
)

TARGET_LINK_LIBRARIES(kadas_gui
  Qt5::Widgets
  Qt5::Network
  Qt5::Xml
  ${QGIS_CORE_LIBRARY}
  ${QGIS_GUI_LIBRARY}
  ${GeographicLib_LIBRARIES}
)

GENERATE_EXPORT_HEADER(
  kadas_gui
  BASE_NAME KADAS_GUI
  EXPORT_FILE_NAME kadas_gui.h
)
SET(kadas_gui_HDR ${kadas_gui_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_gui.h)
