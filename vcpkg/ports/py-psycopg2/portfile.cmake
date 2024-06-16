vcpkg_from_pythonhosted(
    OUT_SOURCE_PATH SOURCE_PATH
    PACKAGE_NAME    psycopg2
    VERSION         ${VERSION}
    SHA512          a691fd09762221e854861dedce37b05e5354e0701feea470a6d5046960056ef02a8c9ecfa751adeba485271ea7d5834643b7d3a3c3f3270087f5ed9c68509f5f
)

vcpkg_add_to_path("${CURRENT_INSTALLED_DIR}/tools/libpq/bin")

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}" OPTIONS -x)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
