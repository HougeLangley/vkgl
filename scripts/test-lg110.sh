#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${1:-"$ROOT/build/hardware"}
ICD_MANIFEST=${2:-"$BUILD_DIR/vkgl_icd_build.json"}

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF -DVKGL_BUILD_HARDWARE_TESTS=ON
cmake --build "$BUILD_DIR" --parallel --target vkgl vkgl-lg110-smoke
"$BUILD_DIR/vkgl-lg110-smoke" "$ICD_MANIFEST"
