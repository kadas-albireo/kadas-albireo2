vcpkg_from_pythonhosted(
    OUT_SOURCE_PATH SOURCE_PATH
    PACKAGE_NAME    fonttools
    VERSION         ${VERSION}
    SHA512          c031bef5cb8850f7d30bbd64b746e5ff4e15d3a11ebc0fc2365bcfcb9c173e09d520a3c82b05aa76d6a8fcb404037f070f937f25baeacf88e98673857aed9700
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${CURRENT_PORT_DIR}/copyright")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
