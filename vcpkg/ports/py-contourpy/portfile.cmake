vcpkg_from_pythonhosted(
    OUT_SOURCE_PATH SOURCE_PATH
    PACKAGE_NAME    contourpy
    VERSION         ${VERSION}
    SHA512          1804b5512cbccef38e86a1d28c91e7a19a72cf9baeaede0873abd7ed7c46f78e3a81a1e7fa87b91cbeef96d55930db6527fbbff905803ad393170e3ed709e0b0
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${CURRENT_PORT_DIR}/copyright")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
