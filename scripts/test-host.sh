#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${1:-"$ROOT/build/host"}
if (($#)); then shift; fi

"$ROOT/scripts/build.sh" "$BUILD_DIR" "$@"
ctest --test-dir "$BUILD_DIR" --output-on-failure -L host-unit
python3 "$ROOT/scripts/check_secrets.py" "$ROOT"
