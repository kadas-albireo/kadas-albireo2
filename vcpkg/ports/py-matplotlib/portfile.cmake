vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO matplotlib/matplotlib
    REF v${VERSION}
    SHA512 c430aa68991cae985416c1d9cd2ffc0ede4756e229d25abbc4349e44ab6d2b2af021326bf1dca928d4d7b65679dcd503a7134de8f685443346a9a76f92ff1655
    HEAD_REF main
)

set(ENV{PKG_CONFIG_PATH} "${CURRENT_INSTALLED_DIR}/lib/pkgconfig;${CURRENT_INSTALLED_DIR}/share/pkgconfig")
set(ENV{INCLUDE} "${CURRENT_INSTALLED_DIR}/include;$ENV{INCLUDE}")

set(PYTHON3 "${CURRENT_HOST_INSTALLED_DIR}/tools/python3/python${VCPKG_HOST_EXECUTABLE_SUFFIX}")
vcpkg_mesonpy_prepare_build_options(OUTPUT meson_opts)

z_vcpkg_setup_pkgconfig_path(CONFIG "RELEASE")

list(APPEND meson_opts  "--python.platlibdir" "${CURRENT_INSTALLED_DIR}/lib")
list(JOIN meson_opts "\",\""  meson_opts)

vcpkg_python_build_and_install_wheel(
  SOURCE_PATH "${SOURCE_PATH}"
  OPTIONS 
    --config-json "{\"setup-args\" : [\"-Dsystem-freetype=true\", \"-Dsystem-qhull=true\", \"${meson_opts}\" ] }" 
  #-Csetup-args=-Dsystem-freetype=true -Csetup-args=-Dsystem-qhull=true
)

file(GLOB licenses "${SOURCE_PATH}/LICENSE/*")

vcpkg_install_copyright(FILE_LIST ${licenses})
string(REPLACE "." ";" version_list "${VERSION}")
list(GET version_list 0 version_major)
list(GET version_list 1 version_minor)
list(GET version_list 2 version_patch)
file(WRITE "${CURRENT_PACKAGES_DIR}/${PYTHON3_SITE}/matplotlib/_version.py"
"\n\
TYPE_CHECKING = False\n\
if TYPE_CHECKING:\n\
    from typing import Tuple, Union\n\
    VERSION_TUPLE = Tuple[Union[int, str], ...]\n\
else:\n\
    VERSION_TUPLE = object\n\
\n\
version: str\n\
__version__: str\n\
__version_tuple__: VERSION_TUPLE\n\
version_tuple: VERSION_TUPLE\n\
\n\
__version__ = version = '${VERSION}'\n\
__version_tuple__ = version_tuple = (${version_major}, ${version_minor}, ${version_patch})\n\
\n\
")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_python_test_import(MODULE "matplotlib")
