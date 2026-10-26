# Install the upstream wheel: building pip-system-certs from source requires a
# newer setuptools than the one of this release
vcpkg_download_distfile(
  WHEEL
  URLS
  "https://files.pythonhosted.org/packages/9f/57/752b63c609affae8f26ae0f1d1103d6ea7e707ad45943f62f7422936071d/pip_system_certs-${VERSION}-py3-none-any.whl"
  FILENAME
  "pip_system_certs-${VERSION}-py3-none-any.whl"
  SHA512
  920c0d4ab601cd608f6647ba8ca49680e309549bd559e751696a753d66cebf4ec301becbd938daa887b981b6a2034f3aa22bc689eaa5f22a4bc69000771c9c0b
)

vcpkg_python_install_wheel(WHEEL "${WHEEL}")

set(LICENSE_FILE "pip_system_certs-${VERSION}.dist-info/licenses/LICENSE")
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

vcpkg_python_test_import(MODULE "pip_system_certs")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
