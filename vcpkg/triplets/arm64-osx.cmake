set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_BUILD_TYPE release)

# qt5-location ships a very old boost version with incompatibilities with clang 16
if(PORT STREQUAL "qt5-location")
  set(VCPKG_CXX_FLAGS "-Wno-enum-constexpr-conversion")
  set(VCPKG_C_FLAGS "-Wno-enum-constexpr-conversion")
endif()
