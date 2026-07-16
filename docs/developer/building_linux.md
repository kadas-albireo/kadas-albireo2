# Building Kadas on Linux

All dependencies (Qt, QGIS, GDAL, Python) are built from source via vcpkg using
the `x64-linux-dynamic` triplet (Release-only). Tested on Ubuntu 22.04 and 24.04.

## 1. System packages

```
sudo apt update
sudo apt install -y build-essential cmake ninja-build git curl zip unzip tar \
  pkg-config autoconf automake libtool autoconf-archive \
  bison flex nasm swig python3 python3-pip mono-complete \
  libgl1-mesa-dev libglu1-mesa-dev '^libxcb.*-dev' libx11-xcb-dev \
  libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev
```

`mono-complete` is needed for the vcpkg NuGet binary cache on Linux.

**Do not install `liburing-dev`.** Qt 6.11 auto-enables its io_uring backend when
it finds the liburing headers, but that backend needs liburing ≥ 2.3 while Ubuntu
22.04 ships 2.1, so it then fails to build (`qioring_linux.cpp`). Leaving
`liburing-dev` uninstalled makes Qt disable the backend on its own.

## 2. Fetch the DTM heightmap data

The Swiss DTM raster (`share/geodata/dtm_analysis.tif`) is not stored in git
(`share/geodata/` is `.gitignore`d). Without it the elevation tools fail with
*"Unable to open heightmap"*, and `make install` fails (`INSTALL_DTM` defaults
`ON`). It is distributed as an OCI artifact and pulled with
[`oras`](https://oras.land/docs/installation) from the repo root:

```
cd /path/to/kadas-albireo2
oras pull ghcr.io/kadas-albireo/kadas-albireo2/dtm_analysis:1.0 -o . # creates share/geodata/dtm_analysis.tif
```

The package is public and only needs pulling once.

## 3. Configure & build

```
cmake -S . -B build -G Ninja \
    -D VCPKG_TARGET_TRIPLET=x64-linux-dynamic \
    -D VCPKG_HOST_TRIPLET=x64-linux-dynamic \
    -D KADAS_VERSION=0.0.1
cmake --build build
```

Note: The first configure builds all dependencies via vcpkg so it takes a very long time (couple of hours).

## 4. Running Kadas

Kadas runs in place from the build tree (no `make install`). It links
against the vcpkg-built libraries, so it needs a few environment variables
pointing at the vcpkg prefix:

```
BUILD=/path/to/build # the -B directory used above
V=$BUILD/vcpkg_installed/x64-linux-dynamic

LD_LIBRARY_PATH=$V/lib:$BUILD/output/lib \
QGIS_PREFIX_PATH=$V \
QT_PLUGIN_PATH=$V/Qt6/plugins \
GDAL_DATA=$V/share/gdal PROJ_DATA=$V/share/proj \
KADAS_DATA_PATH=$(pwd)/share \
$BUILD/output/bin/kadas
```

`KADAS_DATA_PATH` must point at the `share` directory, otherwise
`share/settings_full.ini` (which selects the `CH (online)` startup project) is
not loaded and you get an empty canvas. That project streams basemap tiles from
geo.admin.ch, so it needs internet access.

