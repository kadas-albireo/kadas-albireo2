set(BUILD_DIR "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel")

file(MAKE_DIRECTORY ${BUILD_DIR})

vcpkg_execute_required_process(
        COMMAND "${CURRENT_INSTALLED_DIR}/tools/python3/python${VCPKG_HOST_EXECUTABLE_SUFFIX}" "-m" "pip" "install" "-r" "${CURRENT_PORT_DIR}/requirements.txt" "--prefix" "${CURRENT_PACKAGES_DIR}/tools/python3"
        WORKING_DIRECTORY "${BUILD_DIR}"
        LOGNAME "requirements-install-${TARGET_TRIPLET}-rel"
    )

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
