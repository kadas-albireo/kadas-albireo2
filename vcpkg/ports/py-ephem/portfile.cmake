set(VCPKG_BUILD_TYPE release)

vcpkg_from_pythonhosted(
  OUT_SOURCE_PATH
  SOURCE_PATH
  PACKAGE_NAME
  ephem
  VERSION
  ${VERSION}
  SHA512
  3b5c01e7c1491ab5b7eb28676da74ebbb2235297a33cad5b91340e2056a34eda9075b6ca3f92b4bc1f8bc064ede84bc9c61bda71cb9af345e3969e0316405dfa
)

# Overlay of the open-vcpkg/python-registry port (cmake-formatted); the only
# functional change is this block. _libastro exports all ~155 libastro symbols
# with default visibility, so on ELF its internal calls go through the PLT and
# get interposed by any earlier-loaded global definition. GDAL pulls in
# libnetcdf, which exports its own `range()`; libastro's sunpos() then calls
# netcdf's range() and segfaults (Ephemeris tool crash on Linux). -Bsymbolic
# binds the module's references to its own symbols. (The helper's ENVIRONMENT
# argument is not forwarded, so set ENV directly.)
if(VCPKG_TARGET_IS_LINUX)
  set(ENV{LDFLAGS} "$ENV{LDFLAGS} -Wl,-Bsymbolic")
endif()

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
vcpkg_python_test_import(MODULE "ephem")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
