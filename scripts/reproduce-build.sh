#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if (($# < 2)); then
    printf 'usage: %s BUILD_DIR_A BUILD_DIR_B [CMAKE_ARGS...]\n' "$0" >&2
    exit 2
fi
A=$1
B=$2
shift 2

for dir in "$A" "$B"; do
    cmake -S "$ROOT" -B "$dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON "$@"
    cmake --build "$dir" --parallel --target vkgl
done
sha256sum "$A/libvkgl.so" "$B/libvkgl.so"
cmp "$A/libvkgl.so" "$B/libvkgl.so"
printf 'REPRODUCIBLE_BUILD=PASS\n'
