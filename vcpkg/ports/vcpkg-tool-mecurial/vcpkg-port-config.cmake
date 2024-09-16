# Overwrite builtin scripts
include("${CMAKE_CURRENT_LIST_DIR}/../vcpkg-tool-lessmsi/vcpkg-port-config.cmake")

function(vcpkg_get_mecurial OUT)
    set(version "6.4.2")
    set(tool_names "hg")
    set(tool_subdirectory "tortoisehg-${version}-x64")
    set(download_urls "https://www.mercurial-scm.org/release/tortoisehg/windows/tortoisehg-${version}-x64.msi")
    set(download_filename "tortoisehg-${version}-x64")
    set(download_sha512 f5b665f5c584f611057dc35ae361a9d637da8b286203e3d6b61a29d11c9bf56d3c7ea753142428038fe510b5bcb0c8c7b1a76ec8a8ccb69b8e565eac99f0e4a4)

    find_program(MECURIAL NAMES "${tool_names}" PATHS "${DOWNLOADS}/tools/${tool_subdirectory}")

    if(NOT MECURIAL)
        vcpkg_download_distfile(archive_path
            URLS ${download_urls}
            SHA512 "${download_sha512}"
            FILENAME "${download_filename}"
        )

        set(output_path "${DOWNLOADS}/tools/${tool_subdirectory}")

        file(MAKE_DIRECTORY "${output_path}")
        cmake_path(NATIVE_PATH archive_path archive_path_native) # lessmsi is a bit picky about path formats.
        message(STATUS "Extracting mecurial ...")
        vcpkg_execute_in_download_mode(
                        COMMAND "${LESSMSI}" x "${archive_path_native}" # Using output_path here does not work in bash
                        WORKING_DIRECTORY "${output_path}" 
                        OUTPUT_FILE "${CURRENT_BUILDTREES_DIR}/lessmsi-${TARGET_TRIPLET}-out.log"
                        ERROR_FILE "${CURRENT_BUILDTREES_DIR}/lessmsi-${TARGET_TRIPLET}-err.log"
                        RESULT_VARIABLE error_code
                    )
        if(error_code)
            message(FATAL_ERROR "Couldn't extract mecurial with lessmsi!")
        endif()
        message(STATUS "Extracting mecurial ... finished!")
        file(COPY "${output_path}/tortoisehg-6.4/SourceDir/PFiles/TortoiseHg/" DESTINATION "${output_path}")
        file(REMOVE_RECURSE "${output_path}/tortoisehg-6.4")
        set(MECURIAL "${output_path}/hg.exe")
    endif()

    set(${OUT} "${MECURIAL}" PARENT_SCOPE)
endfunction()
