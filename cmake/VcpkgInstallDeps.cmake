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
            "${SHARE_DIR}/${target}"
  )
endfunction()

if(MSVC)
  set(QGIS_PLUGIN_DIR "${VCPKG_BASE_DIR}/plugins")
  set(QGIS_PYTHON_DIR "${VCPKG_BASE_DIR}/share/qgis/python")
  file(GLOB PROVIDER_LIBS "${QGIS_PLUGIN_DIR}/*provider*.dll")
  file(GLOB AUTHMETHODS_LIBS "${QGIS_PLUGIN_DIR}/*authmethod*.dll")
  # From QGIS CMakeLists.txt
  set(QGIS_PLUGIN_INSTALL_PREFIX "plugins")

  # Additional Qt plugins (3D)
  if(EXISTS "${VCPKG_BASE_DIR}/plugins/renderers/")
    install(DIRECTORY "${VCPKG_BASE_DIR}/plugins/renderers/"
            DESTINATION "bin/plugins/renderers/"
    )
  else()
    message(
      WARNING
        "QGIS 3D renderers plugin directory not found: ${VCPKG_BASE_DIR}/plugins/renderers/"
    )
  endif()
  if(EXISTS "${VCPKG_BASE_DIR}/plugins/renderplugins/")
    install(DIRECTORY "${VCPKG_BASE_DIR}/plugins/renderplugins/"
            DESTINATION "bin/plugins/renderplugins/"
    )
  else()
    message(
      WARNING
        "QGIS 3D renderplugins directory not found: ${VCPKG_BASE_DIR}/plugins/renderplugins/"
    )
  endif()

  # At least python3.dll, qgis_analysis.dll and gsl.dll are missing Copy
  # everything
  file(GLOB ALL_LIBS "${VCPKG_BASE_DIR}/bin/*.dll")
  install(FILES ${ALL_LIBS} DESTINATION "bin")
  install(DIRECTORY "${VCPKG_BASE_DIR}/Qt6/" DESTINATION "bin/Qt6/")
  install(DIRECTORY "${QGIS_PYTHON_DIR}/" DESTINATION "share/qgis/python/")
  install(FILES "${VCPKG_BASE_DIR}/bin/qgis.exe" DESTINATION "bin/")
  install(FILES "${VCPKG_BASE_DIR}/tools/ffmpeg/ffmpeg.exe"
          DESTINATION "opt/ffmpeg"
  )
  install(FILES "${VCPKG_BASE_DIR}/tools/ffmpeg/ffprobe.exe"
          DESTINATION "opt/ffmpeg"
  )
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
          "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins"
)
foreach(LIB ${PROVIDER_LIBS})
  add_custom_command(
    TARGET deploy
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB}"
            "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins"
  )
  install(FILES "${LIB}" DESTINATION "${QGIS_PLUGIN_INSTALL_PREFIX}")
endforeach()
foreach(LIB ${AUTHMETHODS_LIBS})
  add_custom_command(
    TARGET deploy
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB}"
            "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins"
  )
  install(FILES "${LIB}" DESTINATION "${QGIS_PLUGIN_INSTALL_PREFIX}")
endforeach()

# This is needed for gdal to successfully access remote datasets through
# encrypted connections
file(
  DOWNLOAD https://curl.se/ca/cacert.pem "${CMAKE_BINARY_DIR}/cacert.pem"
  TLS_VERIFY ON
  STATUS DOWNLOAD_STATUS
)
list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
if(NOT ${STATUS_CODE} EQUAL 0)
  message(
    FATAL_ERROR "Error occurred during cacert.pem download: ${ERROR_MESSAGE}"
  )
endif()

set(QGIS_SHARE_DIR ${VCPKG_BASE_DIR}/share/qgis)

add_custom_command(
  TARGET deploy
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/cacert.pem"
          "${SHARE_DIR}/cacert.pem"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/qgis/resources"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/qgis/svg"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/qgis/i18n"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${QGIS_SHARE_DIR}/resources"
          "${SHARE_DIR}/qgis/resources"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${QGIS_SHARE_DIR}/svg"
          "${SHARE_DIR}/qgis/svg"
  COMMAND ${CMAKE_COMMAND} -E copy "${QGIS_SHARE_DIR}/i18n/qgis_de.qm"
          "${SHARE_DIR}/qgis/i18n/qgis_de.qm"
  COMMAND ${CMAKE_COMMAND} -E copy "${QGIS_SHARE_DIR}/i18n/qgis_it.qm"
          "${SHARE_DIR}/qgis/i18n/qgis_it.qm"
  COMMAND ${CMAKE_COMMAND} -E copy "${QGIS_SHARE_DIR}/i18n/qgis_fr.qm"
          "${SHARE_DIR}/qgis/i18n/qgis_fr.qm"
  COMMAND ${CMAKE_COMMAND} -E rm -R --
          "${SHARE_DIR}/qgis/resources/cpt-city-qgis-min"
)
set(PROJ_DATA_PATH "${VCPKG_BASE_DIR}/share/proj")

if(NOT EXISTS "${PROJ_DATA_PATH}/proj.db")
  message(FATAL_ERROR "proj.db not found at ${PROJ_DATA_PATH}/proj.db")
endif()

copy_resource("${PROJ_DATA_PATH}" "proj")
copy_resource("${VCPKG_BASE_DIR}/share/gdal" "gdal")
install(DIRECTORY "${SHARE_DIR}/qgis/resources/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/qgis/resources"
)
install(DIRECTORY "${QGIS_SHARE_DIR}/svg/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/qgis/svg"
)
install(DIRECTORY "${QGIS_SHARE_DIR}/i18n/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/qgis/i18n"
)
install(DIRECTORY "${PROJ_DATA_PATH}/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/proj"
)
install(DIRECTORY "${VCPKG_BASE_DIR}/share/gdal/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/gdal"
)
# QCA crypto plugin (qca-ossl). Must be installed under <appdir>/Qt6/plugins,
# which is what main.cpp sets QT_PLUGIN_PATH to, so that QCA can find it via its
# standard "<libraryPath>/crypto/" lookup. Without this, QGIS logs
# "Authentication system DISABLED: QCA's qca-ossl (OpenSSL) plugin is missing".
# The vcpkg Qt6 qca port ships the plugin under bin/Qca-qt6/crypto; we also
# support a couple of fallback layouts to be robust across vcpkg revisions.
set(_qca_candidates
    "${VCPKG_BASE_DIR}/bin/Qca-qt6/crypto"
    "${VCPKG_BASE_DIR}/Qt6/plugins/crypto" "${VCPKG_BASE_DIR}/plugins/crypto"
    "${VCPKG_BASE_DIR}/bin/Qca/crypto"
)
set(_qca_found FALSE)
foreach(_qca_dir IN LISTS _qca_candidates)
  file(GLOB _qca_plugins "${_qca_dir}/qca-ossl*.dll")
  if(_qca_plugins)
    message(STATUS "Installing QCA crypto plugins from ${_qca_dir}")
    install(FILES ${_qca_plugins} DESTINATION "bin/Qt6/plugins/crypto")
    set(_qca_found TRUE)
    break()
  endif()
endforeach()
if(NOT _qca_found)
  message(
    WARNING "QCA qca-ossl plugin not found in any known vcpkg layout; the QGIS "
            "authentication system will remain disabled at runtime. Searched: "
            "${_qca_candidates}"
  )
endif()
install(
  DIRECTORY "${VCPKG_BASE_DIR}/tools/python3/"
  DESTINATION "bin"
  PATTERN "*.sip" EXCLUDE
  PATTERN "__pycache__" EXCLUDE
)

add_dependencies(kadas deploy)
