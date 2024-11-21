file(GLOB kadas_core_SRC *.cpp)
list(SORT kadas_core_SRC)

file(GLOB kadas_core_HDR *.h)
list(SORT kadas_core_HDR)

configure_file(kadas_config.h.in kadas_config.h @ONLY)
set(kadas_core_HDR ${kadas_core_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_config.h)

add_library(kadas_core ${kadas_core_SRC} ${kadas_core_HDR})

find_package(zonedetect REQUIRED)

if(MINGW)
  set(CRYPT_LIBRARIES crypt32 crypto)
elseif(MSVC)
  # OpenSSL should do that since https://github.com/microsoft/vcpkg/pull/18475
  # already
  set(CRYPT_LIBRARIES Crypt32.lib ws2_32.lib)
endif()

target_link_libraries(
  kadas_core
  QGIS::Core
  GDAL::GDAL
  ${CRYPT_LIBRARIES}
  zonedetect::ZoneDetect
  Qt5::Widgets
  Qt5::Network
  Qt5::Svg
  Qt5::Xml
  OpenSSL::SSL
  OpenSSL::Crypto)

target_include_directories(kadas_core PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

generate_export_header(kadas_core BASE_NAME KADAS_CORE EXPORT_FILE_NAME
                       kadas_core.h)
set(kadas_core_HDR ${kadas_core_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_core.h)

install(
  TARGETS kadas_core
  RUNTIME DESTINATION bin COMPONENT libraries
  ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT libraries
  LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT libraries)

install(FILES ${kadas_core_HDR} DESTINATION include/kadas/core)
