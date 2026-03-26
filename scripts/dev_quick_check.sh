#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR" || exit 1

BUILD_DIR="build"

run_step() {
  local label="$1"
  local command="$2"

  echo
  echo "==> ${label}"
  echo "+ ${command}"

  if ! bash -lc "$command"; then
    echo
    echo "[FAIL] ${label}"
    echo "次の行動（再現コマンド）:"
    echo "  ${command}"
    echo
    echo "全体を再実行する場合:"
    echo "  ./scripts/dev_quick_check.sh"
    exit 1
  fi

  echo "[PASS] ${label}"
}

if [ ! -d "$BUILD_DIR" ]; then
  run_step "Configure build directory" "cmake -S . -B ${BUILD_DIR}"
fi

run_step "Docs sync verification" "python scripts/verify_docs_repository_links.py"
run_step "Format check" "cmake --build ${BUILD_DIR} --target format-check"
run_step "Core test (minimum)" "cmake --build ${BUILD_DIR} --target argparse_test example_completion example_manpage && ctest --test-dir ${BUILD_DIR} --output-on-failure -R '^argparse_test$'"

echo
echo "All quick checks passed."
