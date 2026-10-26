# Install the upstream wheel: building pip from source requires a newer
# flit-core than the one of this release
vcpkg_download_distfile(
  WHEEL
  URLS
  "https://files.pythonhosted.org/packages/3f/06/3e4420302363fbb71be3c7f35ce042f54bfca330e3e66b3a2e3c4cca579f/pip-${VERSION}-py3-none-any.whl"
  FILENAME
  "pip-${VERSION}-py3-none-any.whl"
  SHA512
  37d5531dae3dc4d57aed1cea02b67d28419f320b4bc59e60c7e4a706f64f62a5328a77a17632e377db6e0c0cf5685e07275dd7c583ff1f95f37b6c8495d1abdc
)

vcpkg_python_install_wheel(WHEEL "${WHEEL}")

set(LICENSE_FILE "pip-${VERSION}.dist-info/licenses/LICENSE.txt")
file(
  ARCHIVE_EXTRACT
  INPUT
  "${WHEEL}"
  DESTINATION
  "${CURRENT_BUILDTREES_DIR}"
  PATTERNS
  "${LICENSE_FILE}"
)
vcpkg_install_copyright(FILE_LIST "${CURRENT_BUILDTREES_DIR}/${LICENSE_FILE}")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
