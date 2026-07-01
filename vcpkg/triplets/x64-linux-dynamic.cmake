set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_FIXUP_ELF_RPATH ON)

# Build dependencies in Release only (skip Debug) to roughly halve build time
# and disk usage. The Kadas application itself can still be built in any config.
set(VCPKG_BUILD_TYPE release)

# Qt 6.11's io_uring backend fails to compile against liburing < 2.3 (e.g.
# Ubuntu 22.04 ships 2.1): Qt enables the feature just because liburing is
# present, but qioring_linux.cpp then uses newer APIs. io_uring is only a
# performance backend, so disabling it is functionally lossless. Doing it here,
# triplet-wide, lets us use the stock upstream qtbase port instead of vendoring
# a full qtbase overlay just to inject this flag.
#
# This option is appended to *every* port's CMake configure line; ports that
# don't define FEATURE_liburing simply ignore it (a harmless "Manually-specified
# variables were not used by the project" warning at the end of their configure).
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DFEATURE_liburing:BOOL=OFF")
