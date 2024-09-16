vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO python/typing_extensions
    REF ${VERSION}
    SHA512 2d4f19adc5ec9f40502ea6a5c6077b01ea41439fee2b92cd14907e0d093c0f51f4daaba7a2163b5fa14d99413aeb30cad1ef439bb45af6d53634d5ea4ee8a2f4
    HEAD_REF main
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_python_test_import(MODULE "typing_extensions")
