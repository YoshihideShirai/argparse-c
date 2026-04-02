#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

BUILD_DIR="build-coverage"

cmake -S . -B "$BUILD_DIR" \
  -G Ninja \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -DAP_ENABLE_COVERAGE=ON

cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure

ROOT_PATH="${GITHUB_WORKSPACE:-$ROOT_DIR}"
python -m gcovr \
  --root "$ROOT_PATH" \
  --filter "$ROOT_PATH/lib" \
  --object-directory "$ROOT_PATH/$BUILD_DIR" \
  --exclude '.*/test/.*' \
  --exclude '.*/sample/.*' \
  --txt
