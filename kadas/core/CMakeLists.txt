SET(CMAKE_AUTOMOC ON)

FILE(GLOB kadas_core_SRC *.cpp)
LIST(SORT kadas_core_SRC)

FILE(GLOB kadas_core_HDR *.h)
LIST(SORT kadas_core_HDR)

CONFIGURE_FILE(kadas_config.h.in kadas_config.h @ONLY)
SET(kadas_core_HDR ${kadas_core_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_config.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${GDAL_INCLUDE_DIR})

ADD_LIBRARY(kadas_core SHARED
    ${kadas_core_SRC}
    ${kadas_core_HDR}
)

IF(MINGW)
SET(CRYPT_LIBRARIES crypt32 crypto)
ENDIF(MINGW)

TARGET_LINK_LIBRARIES(kadas_core
  Qt5::Widgets
  Qt5::Network
  Qt5::Svg
  Qt5::Xml
  ${QGIS_CORE_LIBRARY}
  ${GDAL_LIBRARIES}
  ${CRYPT_LIBRARIES}
  zonedetect
)

GENERATE_EXPORT_HEADER(
  kadas_core
  BASE_NAME KADAS_CORE
  EXPORT_FILE_NAME kadas_core.h
)
SET(kadas_core_HDR ${kadas_core_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_core.h)

INSTALL(TARGETS kadas_core
  RUNTIME DESTINATION bin COMPONENT libraries
  ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT libraries
  LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT libraries)

INSTALL(FILES ${kadas_core_HDR} DESTINATION include/kadas/core)
