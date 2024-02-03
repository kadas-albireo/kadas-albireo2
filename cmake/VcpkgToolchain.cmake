set(NUGET_SOURCE "https://nuget.pkg.github.com/kadas-albireo/index.json")
set(NUGET_TOKEN "" CACHE STRING "Nuget token")

string(COMPARE EQUAL "${CMAKE_HOST_SYSTEM_NAME}" "Windows" _HOST_IS_WINDOWS)
set(WITH_VCPKG TRUE CACHE BOOL "Use vcpkg for dependency management.")

if(NOT WITH_VCPKG)
  message(STATUS "Building with system libraries --")
  return()
endif()

find_program(VCPKG_EXECUTABLE vcpkg
  HINTS ENV VCPKG_ROOT
)

if(NOT VCPKG_EXECUTABLE)
  message(FATAL_ERROR "WITH_VCPKG is enabled but VCPKG_EXECUTABLE was not found. Make sure vcpkg is installed https://github.com/microsoft/vcpkg-tool/blob/main/README.md#installuseremove and the VCPKG_ROOT environment variable is set.")
endif()

cmake_path(GET VCPKG_EXECUTABLE PARENT_PATH VCPKG_ROOT)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")

# Binarycache can only be used on Windows or if mono is available.
find_program(_VCPKG_MONO mono)
if(NOT "${NUGET_TOKEN}" STREQUAL "" AND (_HOST_IS_WINDOWS OR EXISTS "${_VCPKG_MONO}"))
  execute_process(
    COMMAND ${VCPKG_EXECUTABLE} fetch nuget
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
