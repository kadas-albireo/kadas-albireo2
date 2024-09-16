set(name cppy-${VERSION})
set(wheelname ${name}-py3-none-any.whl)

vcpkg_download_distfile(
    wheel
    URLS https://github.com/nucleic/cppy/releases/download/${VERSION}/${wheelname}
    FILENAME ${wheelname}
    SHA512 033dc1fca6e0c6b75017d64b92208b32da35a1106d9d1fff6520ee27e5472d69be3ba77c22ce46b4d68e21a4166871519ed1f4aebbb130e5d579af6472b20efb
)

vcpkg_python_install_wheel(WHEEL "${wheel}")

vcpkg_install_copyright(FILE_LIST "${CURRENT_PACKAGES_DIR}/${PYTHON3_SITE}/${name}.dist-info/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
