FILE(GLOB kadas_core_SRC *.cpp)
LIST(SORT kadas_core_SRC)

FILE(GLOB kadas_core_HDR *.h)
LIST(SORT kadas_core_HDR)

CONFIGURE_FILE(kadas_config.h.in kadas_config.h @ONLY)
SET(kadas_core_HDR ${kadas_core_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_config.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(kadas_core SHARED
    ${kadas_core_SRC}
    ${kadas_core_HDR}
)

TARGET_LINK_LIBRARIES(kadas_core
  Qt5::Widgets
  Qt5::Network
  ${QGIS_CORE_LIBRARY}
)

GENERATE_EXPORT_HEADER(
  kadas_core
  BASE_NAME KADAS_CORE
  EXPORT_FILE_NAME kadas_core.h
)
SET(kadas_core_HDR ${kadas_core_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_core.h)
