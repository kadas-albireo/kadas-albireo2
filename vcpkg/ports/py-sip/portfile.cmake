vcpkg_from_pythonhosted(
    OUT_SOURCE_PATH SOURCE_PATH
    PACKAGE_NAME    sip
    VERSION         ${VERSION}
    SHA512          8a0ff1c16ead4c2b1c53963a015fb3d149362f649eeec2523e6b243945493f28f5ecf29aabbf5cee5f7909ded40d64a1f469d2d0c1b3c33244e6213ab23ec733
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# Shiver ... where do they come from
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/lib/${python_versioned}/site-packages/pyqtbuild/bundle/dlls/)

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
