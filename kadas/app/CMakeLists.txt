FILE(GLOB kadas_SRC *.cpp)
LIST(SORT kadas_SRC)

FILE(GLOB kadas_HDR *.h)
LIST(SORT kadas_HDR)

FILE(GLOB kadas_UI ui/*.ui)
LIST(SORT kadas_UI)

SET(kadas_RCC_SRC ../resources/resources.qrc)

QT5_ADD_RESOURCES(kadas_RCC ${kadas_RCC_SRC})
QT5_WRAP_UI(kadas_UI_HDR ${kadas_UI})

SET_PROPERTY(SOURCE ${kadas_UI_HDR} PROPERTY SKIP_AUTOMOC ON)
SET_PROPERTY(SOURCE ${kadas_RCC} PROPERTY SKIP_AUTOMOC ON)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE(kadas
    ${kadas_SRC}
    ${kadas_HDR}
    ${kadas_UI}
    ${kadas_UI_HDR}
    ${kadas_RCC_SRC}
    ${kadas_RCC}
)

TARGET_LINK_LIBRARIES(kadas
  Qt5::Widgets
  Qt5::Network
  Qt5::Xml
  ${QGIS_CORE_LIBRARY}
  ${QGIS_GUI_LIBRARY}
  ${CRASHRPT_LIBRARY}
  kadas_core
  kadas_gui
)
