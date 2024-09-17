set(BUILD_DIR "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel")

file(MAKE_DIRECTORY "${BUILD_DIR}")
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/${PYTHON3_SITE}")

if(VCPKG_TARGET_IS_WINDOWS)
  set(PYTHON_EXECUTABLE "${CURRENT_INSTALLED_DIR}/tools/python3/python${VCPKG_HOST_EXECUTABLE_SUFFIX}")
else()
  set(PYTHON_EXECUTABLE "${CURRENT_INSTALLED_DIR}/tools/python3/python${PYTHON3_VERSION_MAJOR}.${PYTHON3_VERSION_MINOR}")
endif()

vcpkg_execute_required_process(
        COMMAND "${PYTHON_EXECUTABLE}" "-m" "pip" "install" "-r" "${CURRENT_PORT_DIR}/requirements.txt" "--prefix" "${CURRENT_PACKAGES_DIR}/tools/python3" --no-build-isolation
        WORKING_DIRECTORY "${BUILD_DIR}"
        LOGNAME "requirements-install-${TARGET_TRIPLET}-rel"
    )

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
