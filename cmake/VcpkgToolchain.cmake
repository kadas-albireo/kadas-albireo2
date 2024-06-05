set(NUGET_SOURCE "https://nuget.pkg.github.com/kadas-albireo/index.json")
set(NUGET_TOKEN "" CACHE STRING "Nuget token")

string(COMPARE EQUAL "${CMAKE_HOST_SYSTEM_NAME}" "Windows" _HOST_IS_WINDOWS)
set(VCPKG_MANIFEST_DIR "${CMAKE_SOURCE_DIR}/vcpkg")

if(NOT VCPKG_TAG STREQUAL VCPKG_INSTALLED_VERSION)
  message(STATUS "Updating vcpkg")
  include(FetchContent)
  FetchContent_Declare(vcpkg
      GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
      GIT_TAG ${VCPKG_TAG}
  )
  FetchContent_MakeAvailable(vcpkg)
else()
  message(STATUS "Using cached vcpkg")
endif()
set(VCPKG_ROOT "${FETCHCONTENT_BASE_DIR}/vcpkg-src" CACHE STRING "")
set(CMAKE_TOOLCHAIN_FILE "${FETCHCONTENT_BASE_DIR}/vcpkg-src/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")

find_package(Git REQUIRED)
if(WIN32)
  execute_process(COMMAND cmd /C "${GIT_EXECUTABLE} -C ${VCPKG_ROOT} rev-parse HEAD" OUTPUT_VARIABLE VCPKG_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  execute_process(COMMAND bash -c "${GIT_EXECUTABLE} -C ${VCPKG_ROOT} rev-parse HEAD" OUTPUT_VARIABLE VCPKG_VERSION ERROR_VARIABLE ERR OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

set(VCPKG_INSTALLED_VERSION ${VCPKG_VERSION} CACHE STRING "" FORCE)

message(STATUS "Building with vcpkg libraries version ${VCPKG_INSTALLED_VERSION}")

# Binarycache can only be used on Windows or if mono is available.
find_program(_VCPKG_MONO mono)
if(NOT "${NUGET_TOKEN}" STREQUAL "" AND (_HOST_IS_WINDOWS OR EXISTS "${_VCPKG_MONO}"))
  # Early bootstrap, copied from the vcpkg toolchain, we need this to fetch nuget
  if(_HOST_IS_WINDOWS)
    set(_VCPKG_EXECUTABLE "${VCPKG_ROOT}/vcpkg.exe")
    set(_VCPKG_BOOTSTRAP_SCRIPT "${VCPKG_ROOT}/bootstrap-vcpkg.bat")
  else()
    set(_VCPKG_EXECUTABLE "${VCPKG_ROOT}/vcpkg")
    set(_VCPKG_BOOTSTRAP_SCRIPT "${VCPKG_ROOT}/bootstrap-vcpkg.sh")
  endif()

  if(NOT EXISTS "${_VCPKG_EXECUTABLE}")
    message(STATUS "Bootstrapping vcpkg before install")

    file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/vcpkg-bootstrap.log" _VCPKG_BOOTSTRAP_LOG)
    execute_process(
      COMMAND "${_VCPKG_BOOTSTRAP_SCRIPT}" ${VCPKG_BOOTSTRAP_OPTIONS}
      OUTPUT_FILE "${_VCPKG_BOOTSTRAP_LOG}"
      ERROR_FILE "${_VCPKG_BOOTSTRAP_LOG}"
      RESULT_VARIABLE _VCPKG_BOOTSTRAP_RESULT)

    if(_VCPKG_BOOTSTRAP_RESULT EQUAL 0)
      message(STATUS "Bootstrapping vcpkg before install - done")
    else()
      message(STATUS "Bootstrapping vcpkg before install - failed")
      file(READ ${_VCPKG_BOOTSTRAP_LOG} MSG)
      message(FATAL_ERROR "vcpkg install failed. See logs for more information: ${MSG}")
    endif()
  endif()

  execute_process(
    COMMAND ${_VCPKG_EXECUTABLE} fetch nuget
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE _FETCH_NUGET_OUTPUT)

  STRING(REGEX REPLACE "\n" ";" _FETCH_NUGET_OUTPUT "${_FETCH_NUGET_OUTPUT}")
  list(GET _FETCH_NUGET_OUTPUT -1 _NUGET_PATH)

  if(_HOST_IS_WINDOWS)
    set(_NUGET_EXE ${_NUGET_PATH})
  else()
    set(_NUGET_EXE ${_VCPKG_MONO} ${_NUGET_PATH})
  endif()

  set(_CONFIG_PATH "${CMAKE_BINARY_DIR}/github-NuGet.Config")

  configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/NuGet.Config.in"
    "${_CONFIG_PATH}"
    @ONLY)
  execute_process(
    COMMAND ${_NUGET_EXE} setapikey "${NUGET_TOKEN}" -src "https://nuget.pkg.github.com/kadas-albireo/index.json" -configfile ${_CONFIG_PATH}
    OUTPUT_VARIABLE _OUTPUT
    ERROR_VARIABLE _ERROR
    RESULT_VARIABLE _RESULT)
  if(_RESULT EQUAL 0)
    message(STATUS "Setup nuget api key - done")
  else()
    message(STATUS "Setup nuget api key - failed")
    message(STATUS "Output:")
    message(STATUS ${_OUTPUT})
    message(STATUS "Error:")
    message(STATUS ${_ERROR})
  endif()

  file(TO_NATIVE_PATH "${_CONFIG_PATH}" _CONFIG_PATH_NATIVE)
  set(ENV{VCPKG_BINARY_SOURCES} "$ENV{VCPKG_BINARY_SOURCES};nugetconfig,${_CONFIG_PATH_NATIVE},readwrite")
endif()

# Copies DLLs built by vcpkg when an install() command is run. Only
# works on Windows.
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON CACHE BOOL "Copy dependency DLLs on install")