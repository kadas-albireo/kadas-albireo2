vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO matplotlib/cycler
    REF v${VERSION}
    SHA512 f1d264de9c5e63515649aefb5937ef7a85d781c07b1c7c8fe291c969565abb18eb48d6d62f77d278746c60900c93700cbb095d280e09de768aedc2463e60d9a2
    HEAD_REF main
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_python_test_import(MODULE "cycler")
