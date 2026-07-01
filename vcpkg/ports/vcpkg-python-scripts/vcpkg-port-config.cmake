include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/../python3/vcpkg-port-config.cmake")

set(PYTHON3_BASEDIR "${CURRENT_INSTALLED_DIR}/tools/python3")
find_program(PYTHON3 NAMES python${PYTHON3_VERSION_MAJOR}.${PYTHON3_VERSION_MINOR} python${PYTHON3_VERSION_MAJOR} python PATHS "${PYTHON3_BASEDIR}" NO_DEFAULT_PATH)
set(PYTHON3_SITEPACKAGES "${CURRENT_PACKAGES_DIR}/${PYTHON3_SITE}")

include("${CMAKE_CURRENT_LIST_DIR}/vcpkg_python_functions.cmake")
