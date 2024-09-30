CMAKE_MINIMUM_REQUIRED(VERSION 3.27.0)

set(VCPKG_TAG e3ad383ceb886677eb181e16adeeb4d4a2c2d4c4)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(WITH_QTWEBKIT ON CACHE BOOL "Use Qt Webkit")
if (WITH_QTWEBKIT)
  list(APPEND VCPKG_MANIFEST_FEATURES "webkit")
endif()

set(WITH_BINDINGS ON CACHE BOOL "Build with python bindings")
if (WITH_BINDINGS)
  list(APPEND VCPKG_MANIFEST_FEATURES "bindings")
endif()

set(WITH_VCPKG TRUE CACHE BOOL "Use the vcpkg submodule for dependency management.")
set(KADAS_SDK_PATH "" CACHE STRING "Build with VCPKG SDK")

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

if (CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message (STATUS "Debug output enabled")
  set(QGISDEBUG TRUE)
else()
  set(QGISDEBUG FALSE)
endif()

PROJECT(kadas LANGUAGES CXX)

set(VCPKG_BASE_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

include(GNUInstallDirs)
include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 17)


SET(KADAS_NAME "Kadas" CACHE STRING "Kadas application name")
SET(KADAS_FULL_NAME "Kadas Albireo" CACHE STRING "Kadas full application name")
SET(KADAS_VERSION_MAJOR "2")
SET(KADAS_VERSION_MINOR "2")
SET(KADAS_VERSION_PATCH "2")
SET(KADAS_VERSION_PRE "")
EXECUTE_PROCESS(COMMAND git rev-parse HEAD OUTPUT_VARIABLE KADAS_VERSION_DEV OUTPUT_STRIP_TRAILING_WHITESPACE)

FIND_PACKAGE(PkgConfig REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5DBus REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)
FIND_PACKAGE(Qt5PrintSupport REQUIRED)
FIND_PACKAGE(Qt5LinguistTools REQUIRED)
FIND_PACKAGE(Qt5Sql REQUIRED)
FIND_PACKAGE(Qt5Svg REQUIRED)
FIND_PACKAGE(Qt5Xml REQUIRED)
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(QGIS REQUIRED COMPONENTS Core Analysis Gui 3D)

FIND_PACKAGE(GDAL CONFIG REQUIRED)
FIND_PACKAGE(QuaZip-Qt5 REQUIRED)
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

if (WITH_QTWEBKIT)
  FIND_PACKAGE(Qt5WebKitWidgets REQUIRED)
  ADD_DEFINITIONS(-DWITH_QTWEBKIT)
endif()

set(CMAKE_AUTOMOC ON)

get_target_property(QMAKE_EXECUTABLE Qt5::qmake LOCATION)

SET(CRASHRPT_LIBRARY "GdbCrashHandler" CACHE FILEPATH "Path to CrashRpt library")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(KADAS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${KADAS_OUTPUT_DIRECTORY}/bin)
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${KADAS_OUTPUT_DIRECTORY}/lib)

FILE(WRITE "${CMAKE_BINARY_DIR}/output/bin/.kadasbuilddir")


SET(MILX_LICENSE_KEY CACHE STRING "")
IF(MILX_LICENSE_KEY STREQUAL "")
  MESSAGE(WARNING "Empty MilX license key, MilX will be unavailable")
ENDIF()

ADD_DEFINITIONS(-DMILX_LICENSE_KEY="${MILX_LICENSE_KEY}")

ADD_SUBDIRECTORY(kadas)

if(WITH_BINDINGS)
  ADD_SUBDIRECTORY(python)
endif()

option(INSTALL_DEMO_DATA "Install settings and templates" OFF)
IF(INSTALL_DEMO_DATA)
  install(FILES "share/settings_patch.ini" DESTINATION "share/kadas/")
  install(DIRECTORY "share/project_templates" DESTINATION "share/kadas/")
ENDIF()

install(DIRECTORY "share/" DESTINATION "share/kadas/"
  PATTERN "share/settings_patch.ini" EXCLUDE
  PATTERN "share/project_templates" EXCLUDE
)

if(WITH_VCPKG)
  message("Install VCPKG deps")
  include(VcpkgInstallDeps)
endif()

include(Package)

IF(UNIX)
    install(FILES com.sourcepole.kadas.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/)
    install(FILES com.sourcepole.kadas.appdata.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo/)
    install(FILES kadas/resources/logo.png RENAME kadas.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps)
ENDIF()
