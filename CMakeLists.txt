cmake_minimum_required(VERSION 3.27.0)

set(VCPKG_TAG e3ad383ceb886677eb181e16adeeb4d4a2c2d4c4)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(WITH_QTWEBKIT
    ON
    CACHE BOOL "Use Qt Webkit"
)
if(WITH_QTWEBKIT)
  list(APPEND VCPKG_MANIFEST_FEATURES "webkit")
endif()

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

project(kadas LANGUAGES CXX)

set(VCPKG_BASE_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

include(GNUInstallDirs)
include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 17)

set(BUILD_SHARED_LIBS
    ON
    CACHE BOOL "Build using shared libraries")
set(KADAS_NAME
    "Kadas"
    CACHE STRING "Kadas application name"
)
set(KADAS_FULL_NAME
    "Kadas Albireo"
    CACHE STRING "Kadas full application name"
)
set(KADAS_VERSION_MAJOR
    "2"
    CACHE STRING "Kadas major version"
)
set(KADAS_VERSION_MINOR
    "3"
    CACHE STRING "Kadas minor version"
)
set(KADAS_VERSION_PATCH
    "0"
    CACHE STRING "Kadas patch version"
)
set(KADAS_VERSION_PRE
    "alpha4"
    CACHE STRING "Kadas pre version"
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

if(WITH_QTWEBKIT)
  find_package(Qt5WebKitWidgets REQUIRED)
  add_definitions(-DWITH_QTWEBKIT)
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

option(INSTALL_DEMO_DATA "Install settings and templates" OFF)
if(INSTALL_DEMO_DATA)
  install(FILES "share/settings_patch.ini" DESTINATION "share/kadas/")
  install(DIRECTORY "share/project_templates" DESTINATION "share/kadas/")
endif()

install(
  DIRECTORY "share/"
  DESTINATION "share/kadas/"
  PATTERN "share/settings_patch.ini" EXCLUDE
  PATTERN "share/project_templates" EXCLUDE
)

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
