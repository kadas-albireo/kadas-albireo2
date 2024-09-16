
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO certifi/python-certifi
    REF 2024.02.02
    SHA512 e62f1741fd9bb10a976f5e864a4946f00e1df1b92082e66fe146ee3275036c365d1e98ed023614a1da07ab2a7a58bc333c77c71586ea50a992eb7d5b54a515e5
    HEAD_REF main
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_python_test_import(MODULE "certifi")
