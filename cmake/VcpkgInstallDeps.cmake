add_custom_target(deploy)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(SHARE_DIR "${CMAKE_BINARY_DIR}/output/bin/kadas.app/Contents/share")
else()
  set(SHARE_DIR "${CMAKE_BINARY_DIR}/output/share")
endif()
file(MAKE_DIRECTORY "${SHARE_DIR}")

function(copy_resource source target)
  add_custom_command(
    TARGET deploy
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/${target}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${source}"
            "${SHARE_DIR}/${target}")
endfunction()

if(MSVC)
  set(QGIS_PLUGIN_DIR "${VCPKG_BASE_DIR}/plugins")
  set(QGIS_PYTHON_DIR "${VCPKG_BASE_DIR}/share/qgis/python")
  file(GLOB PROVIDER_LIBS "${QGIS_PLUGIN_DIR}/*provider*.dll")
  file(GLOB AUTHMETHODS_LIBS "${QGIS_PLUGIN_DIR}/*authmethod*.dll")
  # From QGIS CMakeLists.txt
  set(QGIS_PLUGIN_INSTALL_PREFIX "plugins")

  # Additional Qt plugins (3D)
  install(DIRECTORY "${VCPKG_BASE_DIR}/plugins/renderers/"
          DESTINATION "bin/plugins/renderers/")
  install(DIRECTORY "${VCPKG_BASE_DIR}/plugins/renderplugins/"
          DESTINATION "bin/plugins/renderplugins/")

  # At least python3.dll, qgis_analysis.dll and gsl.dll are missing Copy
  # everything
  file(GLOB ALL_LIBS "${VCPKG_BASE_DIR}/bin/*.dll")
  install(FILES ${ALL_LIBS} DESTINATION "bin")
  install(DIRECTORY "${QGIS_PYTHON_DIR}/" DESTINATION "share/qgis/python/")
  install(FILES "${VCPKG_BASE_DIR}/bin/qgis.exe" DESTINATION "bin/")
  install(FILES "${VCPKG_BASE_DIR}/tools/ffmpeg/ffmpeg.exe" DESTINATION "bin/")
  install(FILES "${VCPKG_BASE_DIR}/tools/ffmpeg/ffprobe.exe" DESTINATION "bin/")
else()
  set(QGIS_PLUGIN_DIR "${VCPKG_BASE_DIR}/lib/qgis/plugins")
  file(GLOB PROVIDER_LIBS "${QGIS_PLUGIN_DIR}/*provider*.so")
  file(GLOB AUTHMETHODS_LIBS "${QGIS_PLUGIN_DIR}/*authmethod*.so")
  # From QGIS CMakeLists.txt
  set(QGIS_PLUGIN_INSTALL_PREFIX "lib${LIB_SUFFIX}/qgis/plugins")
endif()
add_custom_command(
  TARGET deploy
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins")
foreach(LIB ${PROVIDER_LIBS})
  add_custom_command(
    TARGET deploy
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB}"
            "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins")
  install(FILES "${LIB}" DESTINATION "${QGIS_PLUGIN_INSTALL_PREFIX}")
endforeach()
foreach(LIB ${AUTHMETHODS_LIBS})
  add_custom_command(
    TARGET deploy
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB}"
            "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins")
  install(FILES "${LIB}" DESTINATION "${QGIS_PLUGIN_INSTALL_PREFIX}")
endforeach()

# This is needed for gdal to successfully access remote datasets through
# encrypted connections
file(
  DOWNLOAD https://curl.se/ca/cacert.pem "${CMAKE_BINARY_DIR}/cacert.pem"
  TLS_VERIFY ON
  STATUS DOWNLOAD_STATUS)
list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
if(NOT ${STATUS_CODE} EQUAL 0)
  message(
    FATAL_ERROR "Error occurred during cacert.pem download: ${ERROR_MESSAGE}")
endif()

set(QGIS_SHARE_DIR ${VCPKG_BASE_DIR}/share/qgis)

add_custom_command(
  TARGET deploy
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/cacert.pem"
          "${SHARE_DIR}/cacert.pem"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/qgis/resources"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/qgis/svg"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${QGIS_SHARE_DIR}/resources"
          "${SHARE_DIR}/qgis/resources"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${QGIS_SHARE_DIR}/svg"
          "${SHARE_DIR}/qgis/svg"
  COMMAND ${CMAKE_COMMAND} -E rm -R --
          "${SHARE_DIR}/qgis/resources/cpt-city-qgis-min")
set(PROJ_DATA_PATH "${VCPKG_BASE_DIR}/share/proj")

if(NOT EXISTS "${PROJ_DATA_PATH}/proj.db")
  message(FATAL_ERROR "proj.db not found at ${PROJ_DATA_PATH}/proj.db")
endif()

copy_resource("${PROJ_DATA_PATH}" "proj")
copy_resource("${VCPKG_BASE_DIR}/share/gdal" "gdal")
install(DIRECTORY "${SHARE_DIR}/qgis/resources/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/qgis/resources")
install(DIRECTORY "${QGIS_SHARE_DIR}/svg/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/qgis/svg")
install(DIRECTORY "${PROJ_DATA_PATH}/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/proj")
install(DIRECTORY "${VCPKG_BASE_DIR}/share/gdal/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/gdal")
install(DIRECTORY "${VCPKG_BASE_DIR}/bin/Qca/crypto/"
        DESTINATION "bin/plugins/crypto") # QCA plugins
install(
  DIRECTORY "${VCPKG_BASE_DIR}/tools/python3/"
  DESTINATION "bin"
  PATTERN "*.sip" EXCLUDE
  PATTERN "__pycache__" EXCLUDE)

add_dependencies(kadas deploy)
