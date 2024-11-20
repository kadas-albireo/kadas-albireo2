vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  manisandro/svg2svgt
  REF
  v${VERSION}
  SHA512
  9def961080df12725e2f8914d42527926cb9bf791515919b25b6d34ce20373993eed60fa526f3fa778b5cd61f21174914430c0f3aa9fe87513a242d7138ef0dc
  PATCHES
  win.patch)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS "-DWITH_GUI=OFF")

vcpkg_cmake_install()
vcpkg_fixup_pkgconfig()
vcpkg_copy_pdbs()

# file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(
  INSTALL "${SOURCE_PATH}/LICENSE.LGPL"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/"
  RENAME copyright)
