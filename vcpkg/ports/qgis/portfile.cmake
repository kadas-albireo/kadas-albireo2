string(REPLACE "." "_" TAG ${VERSION})

set(QGIS_REF 68dfbb1808e4a8d58e2ead6e83d95d7145638a8d)
set(QGIS_SHA512
    59a0454e91a3b3296dffa7cd4e37ddfb860971ade8dff484e4abcb4b5ca3e1e8b208de0d797edad75ed8f1e0bceba53eec5dcf821fc5c42a1662a45c4fe3d522
)

vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  qgis/QGIS
  REF
  ${QGIS_REF}
  SHA512
  ${QGIS_SHA512}
  HEAD_REF
  master
  PATCHES
  # Make qgis support python's debug library
  qgspython.patch
  libxml2.patch
  exiv2.patch
  crssync.patch
  bigobj.patch
  mesh.patch
  bindings-install.patch
  sipcxx17.patch
  nlohmann-json.patch
  qgis-debug.patch
  # PR #59848
  wms-ignore-reported-layer-extent.patch
)

file(REMOVE ${SOURCE_PATH}/cmake/FindGDAL.cmake)
file(REMOVE ${SOURCE_PATH}/cmake/FindGEOS.cmake)
file(REMOVE ${SOURCE_PATH}/cmake/FindEXIV2.cmake)
file(REMOVE ${SOURCE_PATH}/cmake/FindExpat.cmake)
file(REMOVE ${SOURCE_PATH}/cmake/FindIconv.cmake)
file(REMOVE ${SOURCE_PATH}/cmake/FindPoly2Tri.cmake)

file(REMOVE_RECURSE ${SOURCE_PATH}/external/nlohmann)

vcpkg_find_acquire_program(FLEX)
vcpkg_find_acquire_program(BISON)

vcpkg_backup_env_variables(VARS PATH)

if("bindings" IN_LIST FEATURES)
  # TODO ... we want this to be extracted via python command ?
  vcpkg_add_to_path(PREPEND "${CURRENT_INSTALLED_DIR}/tools/python3/Scripts")
  list(APPEND QGIS_OPTIONS -DWITH_BINDINGS:BOOL=ON)

  list(APPEND QGIS_OPTIONS
       "-DQGIS_PYTHON_DIR=${CURRENT_PACKAGES_DIR}/${PYTHON3_SITE}/qgis"
  )
else()
  vcpkg_find_acquire_program(PYTHON3)
  list(APPEND QGIS_OPTIONS "-DPython_EXECUTABLE=${PYTHON3}")
  list(APPEND QGIS_OPTIONS -DWITH_BINDINGS:BOOL=OFF)
endif()

list(APPEND QGIS_OPTIONS "-DENABLE_TESTS:BOOL=OFF")
list(APPEND QGIS_OPTIONS "-DWITH_GRASS7:BOOL=OFF")
list(APPEND QGIS_OPTIONS "-DWITH_SPATIALITE:BOOL=ON")
list(APPEND QGIS_OPTIONS "-DWITH_QSPATIALITE:BOOL=OFF")
list(APPEND QGIS_OPTIONS "-DWITH_PDAL:BOOL=OFF")
list(APPEND QGIS_OPTIONS "-DWITH_INTERNAL_POLY2TRI:BOOL=OFF")

# Fixup windows install prefixes
list(APPEND QGIS_OPTIONS "-D QGIS_DATA_SUBDIR=share/qgis")
list(APPEND QGIS_OPTIONS "-D QGIS_LIBEXEC_SUBDIR=bin")

list(APPEND QGIS_OPTIONS "-DBISON_EXECUTABLE=${BISON}")
list(APPEND QGIS_OPTIONS "-DFLEX_EXECUTABLE=${FLEX}")
# By default QGIS installs includes into "include" on Windows and into
# "include/qgis" everywhere else let's keep things clean and tidy and put them
# at a predictable location
list(APPEND QGIS_OPTIONS "-DQGIS_INCLUDE_SUBDIR=include/qgis")
list(APPEND QGIS_OPTIONS "-DBUILD_WITH_QT6=OFF")
list(APPEND QGIS_OPTIONS "-DQGIS_MACAPP_FRAMEWORK=FALSE")
# QGIS will also do that starting from protobuf version 4.23
list(APPEND QGIS_OPTIONS "-DProtobuf_LITE_LIBRARY=protobuf::libprotobuf-lite")
list(APPEND QGIS_OPTIONS "-DWITH_INTERNAL_NLOHMANN_JSON:BOOL=OFF")

if("opencl" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DUSE_OPENCL:BOOL=ON)
else()
  list(APPEND QGIS_OPTIONS -DUSE_OPENCL:BOOL=OFF)
endif()

if("gui" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DWITH_GUI:BOOL=ON)
else()
  if("desktop" IN_LIST FEATURES OR "customwidgets" IN_LIST FEATURES)
    message(
      FATAL_ERROR
        "If QGIS is built without gui, desktop and customwidgets cannot be built. Enable gui or disable customwidgets and desktop."
    )
  endif()
  list(APPEND QGIS_OPTIONS -DWITH_GUI:BOOL=OFF)
endif()

if("webkit" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DWITH_QTWEBKIT:BOOL=ON)
else()
  list(APPEND QGIS_OPTIONS -DWITH_QTWEBKIT:BOOL=OFF)
endif()

if("desktop" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DWITH_DESKTOP:BOOL=ON)
else()
  list(APPEND QGIS_OPTIONS -DWITH_DESKTOP:BOOL=OFF)
endif()

if("customwidgets" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DWITH_CUSTOM_WIDGETS:BOOL=ON)
else()
  list(APPEND QGIS_OPTIONS -DWITH_CUSTOM_WIDGETS:BOOL=OFF)
endif()

if("server" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DWITH_SERVER:BOOL=ON)
  if("bindings" IN_LIST FEATURES)
    list(APPEND QGIS_OPTIONS -DWITH_SERVER_PLUGINS:BOOL=ON)
  else()
    list(APPEND QGIS_OPTIONS -DWITH_SERVER_PLUGINS:BOOL=OFF)
  endif()
else()
  list(APPEND QGIS_OPTIONS -DWITH_SERVER:BOOL=OFF)
endif()

if("process" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DWITH_QGIS_PROCESS:BOOL=ON)
else()
  list(APPEND QGIS_OPTIONS -DWITH_QGIS_PROCESS:BOOL=OFF)
endif()

if("3d" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DWITH_3D:BOOL=ON)
else()
  list(APPEND QGIS_OPTIONS -DWITH_3D:BOOL=OFF)
endif()

if("quick" IN_LIST FEATURES)
  list(APPEND QGIS_OPTIONS -DWITH_QUICK:BOOL=ON)
else()
  list(APPEND QGIS_OPTIONS -DWITH_QUICK:BOOL=OFF)
endif()

# Configure debug and release library paths
macro(FIND_LIB_OPTIONS basename relname debname suffix libsuffix)
  file(
    TO_NATIVE_PATH
    "${CURRENT_INSTALLED_DIR}/lib/${VCPKG_TARGET_IMPORT_LIBRARY_PREFIX}${relname}${libsuffix}"
    ${basename}_LIBRARY_RELEASE
  )
  file(
    TO_NATIVE_PATH
    "${CURRENT_INSTALLED_DIR}/debug/lib/${VCPKG_TARGET_IMPORT_LIBRARY_PREFIX}${debname}${libsuffix}"
    ${basename}_LIBRARY_DEBUG
  )
  if(${basename}_LIBRARY_DEBUG
     AND ${basename}_LIBRARY_RELEASE
     AND NOT ${basename}_LIBRARY_DEBUG STREQUAL ${basename}_LIBRARY_RELEASE
  )
    list(APPEND QGIS_OPTIONS_RELEASE
         -D${basename}_${suffix}:FILEPATH=${${basename}_LIBRARY_RELEASE}
    )
    list(APPEND QGIS_OPTIONS_DEBUG
         -D${basename}_${suffix}:FILEPATH=${${basename}_LIBRARY_DEBUG}
    )
  elseif(${basename}_LIBRARY_RELEASE)
    list(APPEND QGIS_OPTIONS
         -D${basename}_${suffix}:FILEPATH=${${basename}_LIBRARY_RELEASE}
    )
  elseif(${basename}_LIBRARY_DEBUG)
    list(APPEND QGIS_OPTIONS
         -D${basename}_${suffix}:FILEPATH=${${basename}_LIBRARY_DEBUG}
    )
  endif()
endmacro()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
  list(APPEND QGIS_OPTIONS -DFORCE_STATIC_LIBS=TRUE)
  # QGIS likes to install auth and providers to different locations on each
  # platform let's keep things clean and tidy and put them at a predictable
  # location
  list(APPEND QGIS_OPTIONS -DQGIS_PLUGIN_SUBDIR=lib)
endif()

if(VCPKG_TARGET_IS_WINDOWS)
  list(
    APPEND
    QGIS_OPTIONS
    -DQT_LRELEASE_EXECUTABLE=${CURRENT_INSTALLED_DIR}/tools/qt5-tools/bin/lrelease.exe
  )
else()
  list(
    APPEND
    QGIS_OPTIONS
    -DQT_LRELEASE_EXECUTABLE=${CURRENT_INSTALLED_DIR}/tools/qt5-tools/bin/lrelease.exe
  )
endif()

vcpkg_backup_env_variables(VARS PATH)

vcpkg_configure_cmake(
  SOURCE_PATH
  ${SOURCE_PATH}
  OPTIONS
  ${QGIS_OPTIONS}
  OPTIONS_DEBUG
  ${QGIS_OPTIONS_DEBUG}
  OPTIONS_RELEASE
  ${QGIS_OPTIONS_RELEASE}
)

vcpkg_restore_env_variables(VARS PATH)

vcpkg_install_cmake()

# if(VCPKG_TARGET_IS_WINDOWS) function(copy_path basepath targetdir) file(GLOB
# ${basepath}_PATH ${CURRENT_PACKAGES_DIR}/${basepath}/*) if( ${basepath}_PATH )
# file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/${targetdir}/qgis/${basepath})
# file(COPY ${${basepath}_PATH} DESTINATION
# ${CURRENT_PACKAGES_DIR}/${targetdir}/qgis/${basepath}) endif()

# if(EXISTS "${CURRENT_PACKAGES_DIR}/${basepath}/") file(REMOVE_RECURSE
# ${CURRENT_PACKAGES_DIR}/${basepath}/) endif() endfunction()

# file(GLOB QGIS_TOOL_PATH
# ${CURRENT_PACKAGES_DIR}/bin/*${VCPKG_TARGET_EXECUTABLE_SUFFIX}
# ${CURRENT_PACKAGES_DIR}/*${VCPKG_TARGET_EXECUTABLE_SUFFIX}) if(QGIS_TOOL_PATH)
# file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/tools/qgis/bin) file(COPY
# ${QGIS_TOOL_PATH} DESTINATION ${CURRENT_PACKAGES_DIR}/tools/qgis/bin)
# file(REMOVE_RECURSE ${QGIS_TOOL_PATH}) file(GLOB QGIS_TOOL_PATH
# ${CURRENT_PACKAGES_DIR}/bin/* ) file(COPY ${QGIS_TOOL_PATH} DESTINATION
# ${CURRENT_PACKAGES_DIR}/tools/qgis/bin) endif()

# file(GLOB QGIS_TOOL_PATH_DEBUG
# ${CURRENT_PACKAGES_DIR}/debug/bin/*${VCPKG_TARGET_EXECUTABLE_SUFFIX}
# ${CURRENT_PACKAGES_DIR}/debug/*${VCPKG_TARGET_EXECUTABLE_SUFFIX})
# if(QGIS_TOOL_PATH_DEBUG) if("debug-tools" IN_LIST FEATURES)
# file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/debug/tools/qgis/bin) file(COPY
# ${QGIS_TOOL_PATH_DEBUG} DESTINATION
# ${CURRENT_PACKAGES_DIR}/debug/tools/qgis/bin) file(REMOVE_RECURSE
# ${QGIS_TOOL_PATH_DEBUG}) file(GLOB QGIS_TOOL_PATH_DEBUG
# ${CURRENT_PACKAGES_DIR}/debug/bin/* ) file(COPY ${QGIS_TOOL_PATH_DEBUG}
# DESTINATION ${CURRENT_PACKAGES_DIR}/debug/tools/qgis/bin) else()
# file(REMOVE_RECURSE ${QGIS_TOOL_PATH_DEBUG}) endif() endif()

# copy_path(doc share) copy_path(i18n share) copy_path(icons share)
# copy_path(images share) copy_path(plugins tools) copy_path(resources share)
# copy_path(svg share)

# # Extend vcpkg_copy_tool_dependencies to support the export of dll and exe
# dependencies in different directories to the same directory, # and support the
# copy of debug dependencies function(vcpkg_copy_tool_dependencies_ex TOOL_DIR
# OUTPUT_DIR SEARCH_DIR) find_program(PS_EXE powershell PATHS ${DOWNLOADS}/tool)
# if (PS_EXE-NOTFOUND) message(FATAL_ERROR "Could not find powershell in vcpkg
# tools, please open an issue to report this.") endif()
# macro(search_for_dependencies PATH_TO_SEARCH) file(GLOB TOOLS
# ${TOOL_DIR}/*.exe ${TOOL_DIR}/*.dll) foreach(TOOL ${TOOLS})
# vcpkg_execute_required_process( COMMAND ${PS_EXE} -noprofile -executionpolicy
# Bypass -nologo -file ${CMAKE_CURRENT_LIST_DIR}/applocal.ps1 -targetBinary
# ${TOOL} -installedDir ${PATH_TO_SEARCH} -outputDir    ${OUTPUT_DIR}
# WORKING_DIRECTORY ${VCPKG_ROOT_DIR} LOGNAME copy-tool-dependencies )
# endforeach() endmacro()
# search_for_dependencies(${CURRENT_PACKAGES_DIR}/${SEARCH_DIR})
# search_for_dependencies(${CURRENT_INSTALLED_DIR}/${SEARCH_DIR}) endfunction()

# vcpkg_copy_tool_dependencies_ex(${CURRENT_PACKAGES_DIR}/tools/${PORT}/bin
# ${CURRENT_PACKAGES_DIR}/tools/${PORT}/bin bin)
# vcpkg_copy_tool_dependencies_ex(${CURRENT_PACKAGES_DIR}/tools/${PORT}/plugins
# ${CURRENT_PACKAGES_DIR}/tools/${PORT}/bin bin) if("debug-tools" IN_LIST
# FEATURES)
# vcpkg_copy_tool_dependencies_ex(${CURRENT_PACKAGES_DIR}/debug/tools/${PORT}/bin
# ${CURRENT_PACKAGES_DIR}/debug/tools/${PORT}/bin debug/bin)
# vcpkg_copy_tool_dependencies_ex(${CURRENT_PACKAGES_DIR}/debug/tools/${PORT}/plugins
# ${CURRENT_PACKAGES_DIR}/debug/tools/${PORT}/bin debug/bin) endif() if("server"
# IN_LIST FEATURES)
# vcpkg_copy_tool_dependencies_ex(${CURRENT_PACKAGES_DIR}/tools/${PORT}/server
# ${CURRENT_PACKAGES_DIR}/tools/${PORT}/bin bin) if("debug-tools" IN_LIST
# FEATURES)
# vcpkg_copy_tool_dependencies_ex(${CURRENT_PACKAGES_DIR}/debug/tools/${PORT}/server
# ${CURRENT_PACKAGES_DIR}/debug/tools/${PORT}/bin debug/bin) endif() endif()
# endif()

vcpkg_restore_env_variables(VARS PATH)

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

# configure_file("${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake.in"
# "${CURRENT_PACKAGES_DIR}/share/${PORT}/vcpkg-cmake-wrapper.cmake" @ONLY)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
