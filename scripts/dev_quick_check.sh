#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR" || exit 1

BUILD_DIR="build"
RUN_COVERAGE=0

print_usage() {
  cat <<USAGE
Usage: ./scripts/dev_quick_check.sh [--with-coverage]

Options:
  --with-coverage  Run CI-compatible coverage measurement via scripts/coverage_ci_gcovr.sh.
  -h, --help       Show this help message.
USAGE
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --with-coverage)
      RUN_COVERAGE=1
      ;;
    -h|--help)
      print_usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      echo
      print_usage
      exit 1
      ;;
  esac
  shift
done

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

if [ "$RUN_COVERAGE" -eq 1 ]; then
  run_step "Coverage (CI-compatible gcovr)" "./scripts/coverage_ci_gcovr.sh"
fi

echo
echo "All quick checks passed."
