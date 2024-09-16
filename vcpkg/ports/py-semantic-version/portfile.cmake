vcpkg_download_distfile(
    wheel
    URLS https://files.pythonhosted.org/packages/6a/23/8146aad7d88f4fcb3a6218f41a60f6c2d4e3a72de72da1825dc7c8f7877c/semantic_version-2.10.0-py2.py3-none-any.whl
    FILENAME semantic_version-2.10.0-py2.py3-none-any.whl
    SHA512 7c9196e00a22bd8a156ed4681a1257a264d2b24ff34c812a3e425edf8f53a76d84450c14f5a9056688797255f6a41655ff2ce1ebcbfc1190009556a4649fafba
)

vcpkg_python_install_wheel(WHEEL "${wheel}")

vcpkg_install_copyright(FILE_LIST "${CURRENT_PACKAGES_DIR}/${PYTHON3_SITE}/semantic_version-2.10.0.dist-info/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
