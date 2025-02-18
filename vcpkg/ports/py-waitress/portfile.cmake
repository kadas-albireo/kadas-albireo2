vcpkg_from_pythonhosted(
    OUT_SOURCE_PATH SOURCE_PATH
    PACKAGE_NAME    waitress
    VERSION         ${VERSION}
    SHA512          f225447b936e4742cf6f0e45b72cc2e33c06ff609c9896fc226de23b9c7ba64140914e3525f57c901617c0a49df3052fe5acbd8ec46f9557832c383ab9d4a483
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
vcpkg_python_test_import(MODULE "waitress")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
