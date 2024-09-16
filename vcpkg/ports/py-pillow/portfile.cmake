vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO python-pillow/Pillow
    REF ${VERSION}
    SHA512 835a8766c384ec3fcf67b42c9bbad35dad0848cc5bd9eba1b0768a864e174a1d9c4a5e989f22496a40f2c29dd7f492f6f80465903fe872b10749cfa0340e1bc5
    HEAD_REF master
)

set(ENV{PKG_CONFIG} "${CURRENT_HOST_INSTALLED_DIR}/tools/pkgconf/pkgconf")
set(ENV{PKG_CONFIG_PATH} "${CURRENT_INSTALLED_DIR}/lib/pkgconfig")
set(ENV{INCLUDE} "${CURRENT_INSTALLED_DIR}/include;$ENV{INCLUDE}")
set(ENV{INCLIB} "${CURRENT_INSTALLED_DIR}/lib;$ENV{INCLIB}")
set(ENV{LIB} "${CURRENT_INSTALLED_DIR}/lib;$ENV{LIB}")

vcpkg_python_build_and_install_wheel(
  SOURCE_PATH "${SOURCE_PATH}" 
  OPTIONS 
    --config-json "{\"raqm\": \"disable\", \"xcb\": \"disable\"}"
    #-C raqm=disable # linkage issues. Without pc file missing linkage to harfbuzz fribidi
)



vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_python_test_import(MODULE "PIL")
