#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${1:-"$ROOT/build/external"}
ICD_MANIFEST=${2:-"$BUILD_DIR/vkgl_icd_build.json"}
if (($#)); then shift; fi
if (($#)); then shift; fi

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF -DVKGL_FETCH_SPIRV_CROSS=ON \
    -DVKGL_BUILD_EXTERNAL_MEMORY_TEST=ON
cmake --build "$BUILD_DIR" --parallel --target vkgl vkgl-external-memory-client
"$BUILD_DIR/vkgl-external-memory-client" "$ICD_MANIFEST" "$@"
