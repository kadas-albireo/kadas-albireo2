set(kadas_core_SRC
    kadas.cpp
    kadasalgorithms.cpp
    kadascoordinateformat.cpp
    kadascoordinateutils.cpp
    kadasfileserver.cpp
    kadaslatlontoutm.cpp
    kadassettingstree.cpp
    kadasstatehistory.cpp
    external/qgis/app/qgsauthesritokenmethod.cpp
)

set(kadas_core_HDR
    kadas.h
    kadasalgorithms.h
    kadascoordinateformat.h
    kadascoordinateutils.h
    kadasfileserver.h
    kadaslatlontoutm.h
    kadaspluginlayer.h
    kadassettingstree.h
    kadasstatehistory.h
    external/qgis/app/qgsauthesritokenmethod.h
)

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
  OpenSSL::Crypto
)

target_include_directories(
  kadas_core
  PUBLIC ${CMAKE_SOURCE_DIR}/kadas/core
         ${CMAKE_SOURCE_DIR}/kadas/core/external/qgis
         ${CMAKE_CURRENT_BINARY_DIR}
)

generate_export_header(
  kadas_core BASE_NAME KADAS_CORE EXPORT_FILE_NAME kadas_core.h
)
set(kadas_core_HDR ${kadas_core_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_core.h)

install(
  TARGETS kadas_core
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib${LIB_SUFFIX}
  LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install(FILES ${kadas_core_HDR} DESTINATION include/kadas/core)
