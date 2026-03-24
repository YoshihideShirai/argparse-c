# セキュリティテスト手順

このページは、`argparse-c` で再現可能なセキュリティ観点の検証手順をまとめたものです。

## CI の根拠

- CI ワークフロー（tests / sanitizers / coverage）: <https://github.com/yoshihideshirai/argparse-c/actions/workflows/ci.yml>
- Pages ワークフロー（coverage 公開）: <https://github.com/yoshihideshirai/argparse-c/actions/workflows/pages.yml>
- Fuzz ワークフロー（smoke / long）: <https://github.com/yoshihideshirai/argparse-c/actions/workflows/fuzz.yml>

## ローカル手順

1. Configure / Build:

   ```bash
   cmake -S . -B build
   cmake --build build --parallel
   ```

2. テスト実行:

   ```bash
   ctest --test-dir build --output-on-failure
   ```

3. sanitizer 構成（CI 相当）:

   ```bash
   cmake -S . -B build-sanitizers -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAP_ENABLE_SANITIZERS=ON
   cmake --build build-sanitizers --parallel
   ctest --test-dir build-sanitizers --output-on-failure
   ```

4. coverage 構成（CI 相当）:

   ```bash
   cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DAP_ENABLE_COVERAGE=ON
   cmake --build build-coverage --parallel
   ctest --test-dir build-coverage --output-on-failure
   ```

## 記載の更新ルール

リリース時に README / FAQ のセキュリティ表現を見直す際は、以下を照合します。

- 最新の CI 実行結果
- 本手順ページ
- 未対応の脆弱性報告の有無
