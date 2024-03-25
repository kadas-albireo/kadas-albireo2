set(CPACK_GENERATOR)
set(CPACK_PACKAGE_EXECUTABLES kadas)
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/kadas-albireo/kadas-albireo2")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")

if(WIN32)
    # message(STATUS "   + WIX                             YES ")
    # set(CPACK_WIX_UPGRADE_GUID 3d1b1ced-39c3-4086-8ab2-4814e0be74df)
    # set(CPACK_WIX_PRODUCT_ICON we need a ico)
    # set(CPACK_WIX_PRODUCT_LOGO "${CMAKE_SOURCE_DIR}/images/icons/kadas.png")
    # set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/windows/template.wxs.in")
    # set(CPACK_WIX_EXTRA_SOURCES "${CMAKE_SOURCE_DIR}/cmake/windows/shortcuts.wxs")
    # set(CPACK_GENERATOR "WIX")
endif()

set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/BundleConfig.cmake")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")

add_custom_target(bundle
                  COMMAND ${CMAKE_CPACK_COMMAND} "--config" "${CMAKE_BINARY_DIR}/BundleConfig.cmake"
                  COMMENT "Running CPACK. Please wait..."
                  DEPENDS kadas)

list(APPEND CPACK_GENERATOR "ZIP")

include(CPack)
