vcpkg_download_distfile(ARCHIVE
    URLS "https://www.riverbankcomputing.com/static/Downloads/QScintilla/2.13.4/QScintilla_src-2.13.4.tar.gz"
    FILENAME "QScintilla-2.13.4.tar.gz"
    SHA512 591379f4d48a6de1bc61db93f6c0d1c48b6830a852679b51e27debb866524c320e2db27d919baf32576c2bf40bba62e38378673a86f22db9839746e26b0f77cd
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    PATCHES
        fix-static.patch
)

file(RENAME "${SOURCE_PATH}/Python/pyproject-qt5.toml" "${SOURCE_PATH}/Python/pyproject.toml")

set(SIPBUILD_ARGS
    "--qmake" "${CURRENT_INSTALLED_DIR}/tools/qt5/bin/qmake${VCPKG_HOST_EXECUTABLE_SUFFIX}"
    "--api-dir" "${CURRENT_PACKAGES_DIR}/share/qt5/qsci/api/python"
    "--qsci-features-dir" "${SOURCE_PATH}/src/features"
    "--qsci-include-dir" "${SITE_PACKAGES}/src"
    "--qsci-library-dir" "${SOURCE_PATH}/src"
    "--no-make"
    "--verbose"
    "--build-dir" "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel"
    "--target-dir" "${CURRENT_INSTALLED_DIR}/${PYTHON3_SITE}"
)

# TODO: help it find sip include dirs, manually patched into sipbuild/project.py for now
# "--sip-include-dirs" "${CURRENT_INSTALLED_DIR}/tools/python3/Lib/site-packages/"

vcpkg_backup_env_variables(VARS PATH)

vcpkg_add_to_path(PREPEND "${CURRENT_HOST_INSTALLED_DIR}/tools/python3/Scripts/")

set(PYTHON3_BASEDIR "${CURRENT_INSTALLED_DIR}/tools/python3")
find_program(PYTHON3 NAMES python${PYTHON3_VERSION_MAJOR}.${PYTHON3_VERSION_MINOR} python${PYTHON3_VERSION_MAJOR} python PATHS "${PYTHON3_BASEDIR}" NO_DEFAULT_PATH)

message(STATUS "Running sipbuild...")
vcpkg_execute_required_process(
    COMMAND "${PYTHON3}" "-m" "sipbuild.tools.build" ${SIPBUILD_ARGS}
    WORKING_DIRECTORY "${SOURCE_PATH}/Python"
    LOGNAME "sipbuild-${TARGET_TRIPLET}"
)
message(STATUS "Running sipbuild...finished.")

# inventory.txt is consumed by the distinfo tool which is run during make and should be run against the package directory
file(TO_NATIVE_PATH "${CURRENT_INSTALLED_DIR}" NATIVE_INSTALLED_DIR)
vcpkg_replace_string("${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/inventory.txt"
        "${CURRENT_INSTALLED_DIR}"
        "${CURRENT_PACKAGES_DIR}")
        vcpkg_replace_string("${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/inventory.txt"
        "${NATIVE_INSTALLED_DIR}"
        "${CURRENT_PACKAGES_DIR}")

vcpkg_qmake_build(SKIP_MAKEFILE BUILD_LOGNAME "install" TARGETS "install")

vcpkg_restore_env_variables(VARS PATH)

vcpkg_python_test_import(MODULE "PyQt5.QtCore")

vcpkg_copy_pdbs()


# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
