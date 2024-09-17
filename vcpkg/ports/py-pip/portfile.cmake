vcpkg_from_pythonhosted(
    OUT_SOURCE_PATH SOURCE_PATH
    PACKAGE_NAME    pip
    VERSION         ${VERSION}
    SHA512          42da0dc6ccf5759fea20fd6f54db272aae7afb2d997c84e1d817e3c95437ba073f4f15cb511e5275cf4f35a82828bb0259c5ffe381d278dc75c2bc8f82dfa404
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
