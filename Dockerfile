FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# ── Core build toolchain ────────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    curl \
    wget \
    zip \
    unzip \
    tar \
    pkg-config \
    ninja-build \
    cmake \
    # C++ extras
    gcc \
    g++ \
    gfortran \
    # Autotools (required by many vcpkg ports)
    autoconf \
    automake \
    libtool \
    libltdl-dev \
    # Flex/Bison (QGIS/GDAL parsers)
    bison \
    flex \
    # Python (used by vcpkg scripts + Qt bindings)
    python3 \
    python3-dev \
    python3-pip \
    # NASM / YASM (ffmpeg, libjpeg-turbo, etc.)
    nasm \
    yasm \
    # gperf (used by some ports)
    gperf \
    # Mono is NOT needed on Linux (Windows only)
    # fdupes for deduplification is NOT needed (macOS CI workaround)
    # swig (Python bindings for QGIS via SIP, not swig, but kept for safety)
    swig \
    # X11 / OpenGL / display (Qt GUI, QGIS require these headers)
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libx11-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxrender-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libx11-xcb-dev \
    libxcb1-dev \
    libxcb-glx0-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxcb-render0-dev \
    libxcb-shape0-dev \
    libxcb-shm0-dev \
    libxcb-sync-dev \
    libxcb-util-dev \
    libxcb-xfixes0-dev \
    libxcb-xinerama0-dev \
    libxcb-xkb-dev \
    libxcb-xinput-dev \
    libxcb-cursor-dev \
    libsm-dev \
    libice-dev \
    libegl1-mesa-dev \
    # dbus (Qt requires it on Linux)
    libdbus-1-dev \
    # fontconfig / freetype (Qt font rendering)
    libfontconfig1-dev \
    libfreetype-dev \
    # Harfbuzz (Qt text shaping)
    libharfbuzz-dev \
    # Input method / accessibility
    libatspi2.0-dev \
    libudev-dev \
    # OpenSSL (network)
    libssl-dev \
    # CUPS (Qt print support)
    libcups2-dev \
    # ALSA (Qt multimedia, optional but often needed)
    libasound2-dev \
    # Misc libs often pulled in by vcpkg deps
    libzstd-dev \
    zlib1g-dev \
    libbz2-dev \
    liblzma-dev \
    libsqlite3-dev \
    # Perl (autoconf scripts)
    perl \
    # Virtual display for running tests
    xvfb \
    # lrelease / lupdate (Qt translations, pulled from vcpkg but just in case)
    && rm -rf /var/lib/apt/lists/*

# ── Upgrade CMake to a version compatible with vcpkg ports ──────────────────
# Ubuntu 24.04 ships 3.28, vcpkg QGIS port wants >= 3.29.
# Pick the right CMake build for the target architecture (x86_64 / aarch64).
ARG CMAKE_VERSION=3.29.6
RUN ARCH="$(uname -m)" \
    && case "${ARCH}" in \
         x86_64)  CMAKE_ARCH=linux-x86_64 ;; \
         aarch64) CMAKE_ARCH=linux-aarch64 ;; \
         *) echo "Unsupported architecture: ${ARCH}" >&2; exit 1 ;; \
       esac \
    && curl -fsSL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-${CMAKE_ARCH}.sh" \
      -o /tmp/cmake.sh \
    && bash /tmp/cmake.sh --skip-license --prefix=/usr/local \
    && rm /tmp/cmake.sh

# ── Non-root user for the build ─────────────────────────────────────────────
RUN useradd -m -s /bin/bash builder
USER builder
WORKDIR /home/builder

# ── Default entrypoint: hand control to the cmake configure + build script ──
COPY --chown=builder:builder docker-build.sh /home/builder/docker-build.sh
RUN chmod +x /home/builder/docker-build.sh

ENTRYPOINT ["/home/builder/docker-build.sh"]
