#!/usr/bin/env bash
# docker-build.sh — Runs inside the Docker container to configure and build kadas.
#
# Usage (from the host, after building the image):
#
#   docker run --rm \
#     -v /path/to/kadas-linux:/src \
#     -v /path/to/vcpkg-cache:/home/builder/.cache/vcpkg \
#     kadas-linux-builder
#
# Environment variables (all optional):
#   TRIPLET        vcpkg triplet  (default: x64-linux-dynamic)
#   BUILD_DIR      build output   (default: /src/build)
#   JOBS           parallel jobs  (default: nproc)
#   SKIP_TESTS     set to 1 to skip ctest

set -euo pipefail

# Auto-select the vcpkg triplet from the container architecture unless the
# caller pinned one explicitly via $TRIPLET.
if [[ -z "${TRIPLET:-}" ]]; then
  case "$(uname -m)" in
    x86_64)  TRIPLET=x64-linux-dynamic ;;
    aarch64) TRIPLET=arm64-linux-dynamic ;;
    *) echo "Unsupported architecture: $(uname -m)" >&2; exit 1 ;;
  esac
fi

SRC_DIR="${SRC_DIR:-/src}"
BUILD_DIR="${BUILD_DIR:-${SRC_DIR}/build}"

# Heavy C++ link steps (Qt, QGIS, GDAL) can each consume several GB of RAM.
# Defaulting to nproc OOM-kills the Docker VM, so cap the default to a value
# that respects the memory available to the container (~2 GB per job), while
# still allowing an explicit override via $JOBS.
if [[ -z "${JOBS:-}" ]]; then
  CPUS="$(nproc)"
  # MemTotal is in kB; allow ~2 GB per parallel job.
  MEM_KB="$(awk '/MemTotal/ {print $2}' /proc/meminfo 2>/dev/null || echo 0)"
  MEM_JOBS=$(( MEM_KB / 1024 / 1024 / 2 ))
  [[ "${MEM_JOBS}" -lt 1 ]] && MEM_JOBS=1
  if [[ "${MEM_JOBS}" -lt "${CPUS}" ]]; then
    JOBS="${MEM_JOBS}"
  else
    JOBS="${CPUS}"
  fi
fi

echo "=== kadas Linux build ==="
echo "  SRC:     ${SRC_DIR}"
echo "  BUILD:   ${BUILD_DIR}"
echo "  TRIPLET: ${TRIPLET}"
echo "  JOBS:    ${JOBS}"
echo "========================="

cd "${SRC_DIR}"

# ── Configure ───────────────────────────────────────────────────────────────
cmake -S . \
      -G Ninja \
      -B "${BUILD_DIR}" \
      -D CMAKE_BUILD_TYPE=Release \
      -D VCPKG_TARGET_TRIPLET="${TRIPLET}" \
      -D VCPKG_HOST_TRIPLET="${TRIPLET}"

# ── Build ────────────────────────────────────────────────────────────────────
cmake --build "${BUILD_DIR}" -j "${JOBS}"

# ── Tests ────────────────────────────────────────────────────────────────────
if [[ "${SKIP_TESTS:-0}" != "1" ]]; then
  ctest --test-dir "${BUILD_DIR}" --output-on-failure --timeout 120
fi

echo "=== Build complete ==="
