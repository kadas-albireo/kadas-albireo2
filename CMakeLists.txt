cmake_minimum_required(VERSION 3.27.0)

set(VCPKG_TAG e3ad383ceb886677eb181e16adeeb4d4a2c2d4c4)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(WITH_BINDINGS
    ON
    CACHE BOOL "Build with python bindings"
)
if(WITH_BINDINGS)
  list(APPEND VCPKG_MANIFEST_FEATURES "bindings")
endif()

set(WITH_VCPKG
    TRUE
    CACHE BOOL "Use the vcpkg submodule for dependency management."
)
set(KADAS_SDK_PATH
    ""
    CACHE STRING "Build with VCPKG SDK"
)

if(NOT KADAS_SDK_PATH STREQUAL "")
  message(STATUS "Building with KADAS SDK -- ${KADAS_SDK_PATH}")
  set(CMAKE_TOOLCHAIN_FILE "${KADAS_SDK_PATH}/scripts/buildsystems/vcpkg.cmake")
  set(WITH_VCPKG ON)
elseif(WITH_VCPKG)
  message(STATUS "Building local dependencies with VCPKG --")
  include(VcpkgToolchain)
else()
  message(STATUS "Building with system libraries --")
endif()

set(KADAS_VERSION
    "0.0.1"
    CACHE STRING "KADAS Version"
)

string(REPLACE "." ";" VERSION_LIST ${KADAS_VERSION})
list(GET VERSION_LIST 0 KADAS_VERSION_MAJOR)
list(GET VERSION_LIST 1 KADAS_VERSION_MINOR)
list(GET VERSION_LIST 2 KADAS_VERSION_PATCH)

message(STATUS "Building KADAS version ${KADAS_VERSION}")

set(KADAS_NAME
    "Kadas"
    CACHE STRING "Kadas application name"
)
set(KADAS_FULL_NAME
    "Kadas Albireo"
    CACHE STRING "Kadas full application name"
)

project(
  kadas
  LANGUAGES CXX
  VERSION "${KADAS_VERSION}"
)

set(VCPKG_BASE_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

include(GNUInstallDirs)
include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 17)

set(BUILD_SHARED_LIBS
    ON
    CACHE BOOL "Build using shared libraries"
)

execute_process(
  COMMAND git rev-parse HEAD
  OUTPUT_VARIABLE KADAS_VERSION_DEV
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_package(PkgConfig REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(QGIS REQUIRED COMPONENTS Core Analysis Gui 3D)

find_package(GDAL CONFIG REQUIRED)
find_package(QuaZip-Qt5 REQUIRED)
find_package(OpenSSL REQUIRED)
if(WITH_BINDINGS)
  find_package(Python 3.7 REQUIRED COMPONENTS Interpreter Development)
  find_package(QGIS REQUIRED COMPONENTS Python)

  if(WIN32)
    set(SITEARCH_INSTALL_DIR "bin/Lib/site-packages")
  else()
    set(SITEARCH_INSTALL_DIR "${Python_SITEARCH}")
  endif()
endif()

set(CMAKE_AUTOMOC ON)

get_target_property(QMAKE_EXECUTABLE Qt5::qmake LOCATION)

set(CRASHRPT_LIBRARY
    "GdbCrashHandler"
    CACHE FILEPATH "Path to CrashRpt library"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(KADAS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${KADAS_OUTPUT_DIRECTORY}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${KADAS_OUTPUT_DIRECTORY}/lib)

include(InstallDeps)

file(WRITE "${CMAKE_BINARY_DIR}/output/bin/.kadasbuilddir" )

set(MILX_LICENSE_KEY CACHE STRING "")
if(MILX_LICENSE_KEY STREQUAL "")
  message(WARNING "Empty MilX license key, MilX will be unavailable")
endif()

add_definitions(-DMILX_LICENSE_KEY="${MILX_LICENSE_KEY}")

add_subdirectory(kadas)

if(WITH_BINDINGS)
  add_subdirectory(python)
endif()

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

set(INSTALL_SWISS_DTM
    TRUE
    CACHE BOOL "Install dtm_swiss_only.tif."
)
if(INSTALL_SWISS_DTM)
  install(FILES "share/geodata/dtm_swiss_only.tif"
          DESTINATION "share/kadas/geodata/"
  )
endif()
set(INSTALL_ALBIREO_SETTINGS
    TRUE
    CACHE BOOL "Install Albireo Settings."
)
if(INSTALL_ALBIREO_SETTINGS)
  install(FILES "share/settings_patch.ini" DESTINATION "share/kadas/")
  install(DIRECTORY "share/project_templates" DESTINATION "share/kadas/")
endif()

# install( DIRECTORY "share/" DESTINATION "share/kadas/" PATTERN
"settings_patch.ini" EXCLUDE PATTERN "project_templates" EXCLUDE
PATTERN "geodata/" EXCLUDE )

if(WITH_VCPKG)
  message("Install VCPKG deps")
  include(VcpkgInstallDeps)
endif()

include(Package)

if(UNIX)
  install(FILES com.sourcepole.kadas.desktop
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/
  )
  install(FILES com.sourcepole.kadas.appdata.xml
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo/
  )
  install(
    FILES kadas/resources/logo.png
    RENAME kadas.png
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps
  )
endif()
