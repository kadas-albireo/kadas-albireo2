set(CPACK_GENERATOR)
set(CPACK_PACKAGE_EXECUTABLES "kadas;KADAS Albireo")
set(CPACK_PACKAGE_HOMEPAGE_URL
    "https://github.com/kadas-albireo/kadas-albireo2"
)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

set(CPACK_PACKAGE_VENDOR "OPENGIS.ch")
set(CPACK_PACKAGE_NAME "KADAS Albireo")

if(WIN32)
  set(CREATE_MSI
      OFF
      CACHE BOOL "Create a .msi installer (windows only)"
  )
  if(CREATE_MSI)
    message(STATUS "   + WIX                             YES ")
    # This is for WiX so that it does not complain about unsupported WiX License
    # file extension.
    configure_file(
      "${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_BINARY_DIR}/LICENSE.txt" COPYONLY
    )
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")

    set(CPACK_WIX_UPGRADE_GUID 3d1b1ced-39c3-4086-8ab2-4814e0be74df)
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/kadas/resources/logo.ico")
    configure_file("${CMAKE_SOURCE_DIR}/cmake/shortcuts.wxs.in" "${CMAKE_BINARY_DIR}/shortcuts.wxs" @ONLY)
    set(CPACK_WIX_EXTRA_SOURCES "${CMAKE_BINARY_DIR}/shortcuts.wxs")
    list(APPEND CPACK_GENERATOR "WIX")
  endif()
endif()

set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/BundleConfig.cmake")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "KadasAlbireo")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")

add_custom_target(
  bundle
  COMMAND ${CMAKE_CPACK_COMMAND} "--config"
          "${CMAKE_BINARY_DIR}/BundleConfig.cmake"
  COMMENT "Running CPACK. Please wait..."
  DEPENDS kadas
)

list(APPEND CPACK_GENERATOR "ZIP")

include(CPack)
