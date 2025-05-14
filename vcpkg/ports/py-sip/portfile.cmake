vcpkg_from_pythonhosted(
  OUT_SOURCE_PATH
  SOURCE_PATH
  PACKAGE_NAME
  sip
  VERSION
  ${VERSION}
  SHA512
  499339424eccb27309ca2d220cf57c29b484faed45849ea2ab7772a69841b617ab01a8261e70869e7071f4871bc7211a7bb421cc2ff30d0fcf46f28e4c41f323
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# Shiver ... where do they come from
file(
  REMOVE_RECURSE
  ${CURRENT_PACKAGES_DIR}/lib/${python_versioned}/site-packages/pyqtbuild/bundle/dlls/
)

if(NOT VCPKG_TARGET_IS_WINDOWS)
  vcpkg_fixup_shebang(SCRIPT "bin/sip-build")
  vcpkg_fixup_shebang(SCRIPT "bin/sip-distinfo")
  vcpkg_fixup_shebang(SCRIPT "bin/sip-install")
  vcpkg_fixup_shebang(SCRIPT "bin/sip-module")
  vcpkg_fixup_shebang(SCRIPT "bin/sip-sdist")
  vcpkg_fixup_shebang(SCRIPT "bin/sip-wheel")
else()
  set(WRAPPER_BAT "${CURRENT_PACKAGES_DIR}/tools/python3/Scripts/sip-build.bat")
  file(WRITE "${WRAPPER_BAT}" "@echo off\n")
  file(APPEND "${WRAPPER_BAT}" "set SCRIPT_DIR=%~dp0\n\n")
  file(APPEND "${WRAPPER_BAT}" "set PYTHON_EXE=%SCRIPT_DIR%..\\python.exe\n\n")
  file(APPEND "${WRAPPER_BAT}" "\"%PYTHON_EXE%\" -m sipbuild.tools.build %*\n")
endif()

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
