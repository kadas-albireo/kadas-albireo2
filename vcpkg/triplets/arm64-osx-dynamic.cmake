set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

set(VCPKG_BUILD_TYPE release)

# Xcode 26 uses ld-prime which breaks old qmake -L paths (e.g. /usr/X11R6/lib)
set(VCPKG_LINKER_FLAGS "-Wl,-ld_classic")
