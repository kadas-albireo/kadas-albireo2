# Building Kadas on Linux

## Overview

All dependencies (Qt, QGIS, GDAL, Python) are built from source through
vcpkg using the `x64-linux-dynamic` triplet.

Versions tested: Ubuntu 22.04 (Jammy)

## 1. System packages

```
sudo apt update
sudo apt install -y build-essential cmake ninja-build git curl zip unzip tar \
  pkg-config autoconf automake libtool autoconf-archive \
  bison flex nasm swig python3 python3-pip mono-complete \
  libgl1-mesa-dev libglu1-mesa-dev '^libxcb.*-dev' libx11-xcb-dev \
  libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev
```

`mono-complete` is needed for the vcpkg NuGet binary cache on Linux. No
`liburing-dev` is required as Qt's io_uring backend is disabled triplet-wide (see
[Troubleshooting → io_uring](#qtbase-fails-io_uring-not-declared)).

## 2. Fetch the DTM heightmap data

The Swiss DTM raster (`share/geodata/dtm_analysis.tif`) is **not stored in git**
(and `share/geodata/` is `.gitignore`d). It's distributed as an OCI artifact in
the GitHub Container Registry and pulled with [`oras`](https://oras.land).
Without it the elevation tools (height profile, hillshade, slope, viewshed,
line-of-sight) fail with *"Unable to open heightmap"*, and an install with
`-D INSTALL_DTM=ON` (the default) fails because the file to install is missing.

Install `oras` (single static binary — see https://oras.land/docs/installation),
then pull the raster from the **repo root** so it lands at the expected path:

```
cd /path/to/kadas-albireo2
oras pull ghcr.io/kadas-albireo/kadas-albireo2/dtm_analysis:1.0 -o .
# creates share/geodata/dtm_analysis.tif
```

The package is public, so no GHCR authentication is needed. Only has to be done
once (re-pull if the artifact version is bumped). The WMS basemaps (Landeskarten,
SWISSIMAGE) stream from swisstopo and are unaffected.

## 3. Configure & build

```
cmake -S . -B build -G Ninja \
    -D VCPKG_TARGET_TRIPLET=x64-linux-dynamic \
    -D VCPKG_HOST_TRIPLET=x64-linux-dynamic \
    -D KADAS_VERSION=0.0.1
```

The first configure builds **all** dependencies via vcpkg. This takes a long
time and a lot of disk space (tens of GB). Dependencies are built Release-only
by the triplet, which roughly halves the footprint.

Once configure finishes (`-- Build files have been written to: <build>`),
compile Kadas itself:

```
cmake --build build
```

This step is quicker as the dependencies are already built. The
resulting binary is `<build>/output/bin/kadas` (it is **not** placed in the
source tree). 


## 4. Running Kadas

The build is run **in place** from the build tree (i.e no `make install` is needed).
Because the binary links against the vcpkg-built Qt/QGIS/GDAL/Python libraries,
it needs several environment variables pointing at the vcpkg prefix
(`<build>/vcpkg_installed/x64-linux-dynamic`):

```
BUILD=/path/to/build                       # the -B directory used above
V=$BUILD/vcpkg_installed/x64-linux-dynamic

LD_LIBRARY_PATH=$V/lib:$BUILD/output/lib \
QGIS_PREFIX_PATH=$V \
QT_PLUGIN_PATH=$V/Qt6/plugins \
GDAL_DATA=$V/share/gdal PROJ_DATA=$V/share/proj \
KADAS_DATA_PATH=$(pwd)/share \
$BUILD/output/bin/kadas
```

`KADAS_DATA_PATH` must point at the `share` directory: at runtime
`Kadas::pkgDataPath()` otherwise looks under `<repo>/data`, so
`share/settings_full.ini` (which selects the `CH (online).qgz` startup project)
is never loaded and you get an empty canvas. That startup project also streams
basemap tiles from geo.admin.ch / maps.ch, so it needs **internet access**.

## Build options reference

Both default to `ON` and matter mainly for `make install` (not for running in
place):

- `INSTALL_DTM` installs `share/geodata/dtm_analysis.tif` to
  `share/kadas/geodata/`. With the default `ON`, `make install` **fails** if the
  DTM has not been fetched (see [step 2](#2-fetch-the-dtm-heightmap-data)). Set
  `-D INSTALL_DTM=OFF` to skip it.
- `INSTALL_ALBIREO_SETTINGS` installs `share/settings_patch.ini` and
  `share/project_templates/` (the `CH` startup project) to `share/kadas/`.