vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO pypa/packaging
    REF ${VERSION}
    SHA512 45836ea0e7531c485a7d5e568f9740c7f075ff1b72a0d60422c5b76c031866428ff2137d9bd153790a9fcfff6bd4a7555bc40555409ad46debae7f1c9c1fe129
    HEAD_REF main
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_python_test_import(MODULE "packaging")
