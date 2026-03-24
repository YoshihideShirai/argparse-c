# Security testing procedure

This page summarizes the reproducible security-focused checks used for `argparse-c`.

## CI evidence

- CI workflow (tests, sanitizers, coverage): <https://github.com/yoshihideshirai/argparse-c/actions/workflows/ci.yml>
- Pages workflow (coverage publication): <https://github.com/yoshihideshirai/argparse-c/actions/workflows/pages.yml>
- Fuzz workflow (smoke/long): <https://github.com/yoshihideshirai/argparse-c/actions/workflows/fuzz.yml>

## Local procedure

1. Configure and build:

   ```bash
   cmake -S . -B build
   cmake --build build --parallel
   ```

2. Run tests:

   ```bash
   ctest --test-dir build --output-on-failure
   ```

3. Run sanitizer configuration (as used in CI):

   ```bash
   cmake -S . -B build-sanitizers -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAP_ENABLE_SANITIZERS=ON
   cmake --build build-sanitizers --parallel
   ctest --test-dir build-sanitizers --output-on-failure
   ```

4. Run coverage configuration (as used in CI):

   ```bash
   cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DAP_ENABLE_COVERAGE=ON
   cmake --build build-coverage --parallel
   ctest --test-dir build-coverage --output-on-failure
   ```

## Statement maintenance rule

At each release, the security-related wording in README/FAQ should be reviewed against:

- the latest CI run status,
- this procedure page,
- and any open vulnerability reports.
