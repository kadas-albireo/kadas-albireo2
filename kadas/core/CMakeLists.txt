FILE(GLOB kadas_core_SRC *.cpp)
LIST(SORT kadas_core_SRC)

FILE(GLOB kadas_core_HDR *.h)
LIST(SORT kadas_core_HDR)

CONFIGURE_FILE(kadas_config.h.in kadas_config.h @ONLY)
SET(kadas_core_HDR ${kadas_core_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_config.h)

ADD_LIBRARY(kadas_core
    ${kadas_core_SRC}
    ${kadas_core_HDR}
)

find_package(zonedetect REQUIRED)

if(MINGW)
  set(CRYPT_LIBRARIES crypt32 crypto)
elseif(MSVC)
  # OpenSSL should do that since https://github.com/microsoft/vcpkg/pull/18475 already
  set(CRYPT_LIBRARIES Crypt32.lib ws2_32.lib)
endif()

TARGET_LINK_LIBRARIES(kadas_core
  QGIS::Core
  GDAL::GDAL
  ${CRYPT_LIBRARIES}
  zonedetect::ZoneDetect
  Qt5::Widgets
  Qt5::Network
  Qt5::Svg
  Qt5::Xml
  OpenSSL::SSL
  OpenSSL::Crypto
)

target_include_directories(kadas_core ${CMAKE_CURRENT_BINARY_DIR})

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
