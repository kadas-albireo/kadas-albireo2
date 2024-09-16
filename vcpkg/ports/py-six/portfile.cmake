vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO benjaminp/six
    REF ${VERSION}
    SHA512 630179b9994e10573225dcfa4ff7a40449a58bbe57ccff06434fa40ded10c6e3e1d72a230860a8e6c839c7c17357aca9e1f38aede9566782339331eef65fed3a
    HEAD_REF master
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_python_test_import(MODULE "six")
