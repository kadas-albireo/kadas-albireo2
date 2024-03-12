if(NOT WITH_VCPKG)
  return()
endif()

# Copy files from the install dir to where it
# will be bundled
add_custom_target(deploy)

function(copy_resource source target)
add_custom_command(TARGET deploy
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/${target}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${source}" "${SHARE_DIR}/${target}"
)
endfunction()

set(VCPKG_BASE_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
if(MSVC)
    set(QGIS_PLUGIN_DIR "${VCPKG_BASE_DIR}/tools/qgis/plugins")
    file(GLOB PROVIDER_LIBS
        "${QGIS_PLUGIN_DIR}/*provider*.dll"
    )
    file(GLOB AUTHMETHODS_LIBS
        "${QGIS_PLUGIN_DIR}/*authmethod*.dll"
    )
    # From QGIS CMakeLists.txt
    set(QGIS_PLUGIN_INSTALL_PREFIX "plugins")
else()
    set(QGIS_PLUGIN_DIR "${VCPKG_BASE_DIR}/lib/qgis/plugins")
    file(GLOB PROVIDER_LIBS
        "${QGIS_PLUGIN_DIR}/*provider*.so"
    )
    file(GLOB AUTHMETHODS_LIBS
        "${QGIS_PLUGIN_DIR}/*authmethod*.so"
    )
    # From QGIS CMakeLists.txt
    set(QGIS_PLUGIN_INSTALL_PREFIX "lib${LIB_SUFFIX}/qgis/plugins")
endif()
add_custom_command(TARGET deploy
POST_BUILD
COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins"
)
foreach(LIB ${PROVIDER_LIBS})
add_custom_command(TARGET deploy
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB}" "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins"
)
install(FILES "${LIB}" DESTINATION "${QGIS_PLUGIN_INSTALL_PREFIX}")
endforeach()
foreach(LIB ${AUTHMETHODS_LIBS})
add_custom_command(TARGET deploy
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB}" "${CMAKE_BINARY_DIR}/output/bin/qgis/plugins"
)
endforeach()

# This is needed for gdal to successfully access remote datasets through encrypted connections
file(DOWNLOAD https://curl.se/ca/cacert.pem "${CMAKE_BINARY_DIR}/cacert.pem" TLS_VERIFY ON STATUS DOWNLOAD_STATUS)
list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
if(NOT ${STATUS_CODE} EQUAL 0)
    message(FATAL_ERROR "Error occurred during download: ${ERROR_MESSAGE}")
endif()

add_custom_command(TARGET deploy
POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/cacert.pem" "${SHARE_DIR}/cacert.pem"
COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/qgis/resources"
COMMAND ${CMAKE_COMMAND} -E make_directory "${SHARE_DIR}/qgis/svg"
COMMAND ${CMAKE_COMMAND} -E copy_directory "${VCPKG_BASE_DIR}/share/qgis/resources" "${SHARE_DIR}/qgis/resources"
COMMAND ${CMAKE_COMMAND} -E copy_directory "${VCPKG_BASE_DIR}/share/qgis/svg" "${SHARE_DIR}/qgis/svg"
COMMAND ${CMAKE_COMMAND} -E rm -- "${SHARE_DIR}/qgis/resources/data/world_map.gpkg"
COMMAND ${CMAKE_COMMAND} -E rm -R -- "${SHARE_DIR}/qgis/resources/cpt-city-qgis-min"
)
set(PROJ_DATA_PATH "${VCPKG_BASE_DIR}/share/proj")

if(NOT EXISTS "${PROJ_DATA_PATH}/proj.db")
    message(FATAL_ERROR "proj.db not found at ${PROJ_DATA_PATH}/proj.db")
endif()

copy_resource("${PROJ_DATA_PATH}" "proj")
copy_resource("${VCPKG_BASE_DIR}/share/gdal" "gdal")
copy_resource("${CMAKE_SOURCE_DIR}/packaging/files" "kadas")
install(DIRECTORY "${VCPKG_BASE_DIR}/share/qgis/resources/" DESTINATION "${CMAKE_INSTALL_DATADIR}/qgis/resources")
install(DIRECTORY "${VCPKG_BASE_DIR}/share/qgis/svg/" DESTINATION "${CMAKE_INSTALL_DATADIR}/qgis/svg")
install(DIRECTORY "${PROJ_DATA_PATH}/" DESTINATION "${CMAKE_INSTALL_DATADIR}/proj/")
install(DIRECTORY "${VCPKG_BASE_DIR}/share/gdal/" DESTINATION "${CMAKE_INSTALL_DATADIR}/gdal")

add_dependencies(kadas deploy)