# known args / unknown args

`ap_parse_known_args(...)` は、既知引数だけをパースし、未知トークンを別配列へ回収したいときに使います。

## 使いどころ

- ラッパー CLI を作る場合
- 一部だけ自前で解釈し、残りは別コマンドへ渡す場合
- `--` 以降をそのまま保持したい場合

## API

```c
ap_parse_known_args(parser, argc, argv, &ns, &unknown, &unknown_count, &err);
```

## unknown 側に回収されるもの

- 未知オプション
- 未知オプションの直後にある値らしいトークン
- positional 割り当て後に余ったトークン
- `--` 以降のすべてのトークン

## `--` 区切りあり/なしの具体例

ラッパー CLI では、下流へ渡す引数を **unknown 候補として扱う** のか、
**`--` 以降を完全パススルーにする** のかを明示すると安全です。

`--` なし:

```text
wrapper --config conf.yml --plugin-opt x --verbose input.txt
```

- `--plugin-opt` と `x` は unknown として回収されます。
- `--verbose` がラッパー側の既知オプションなら、引き続きラッパーで解釈されます。
- 「unknown を拾いつつ、既知オプションは最後まで処理したい」場合に向きます。

`--` あり:

```text
wrapper --config conf.yml -- --plugin-opt x --verbose input.txt
```

- `--` 以降は順序そのままで unknown に回収されます。
- ラッパー側の解釈境界を明確にできます。
- 別実行ファイルへ安全に引数転送したい場合のデフォルトとして推奨です。

根拠テスト:

- [`ParseKnownArgsCollectsAfterDoubleDash`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- [`ParseKnownArgsDoubleDashProtectsKnownFlagsFromAccidentalInjection`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- [`ParseKnownArgsWithoutDoubleDashStillParsesKnownFlags`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)

## unknown 引数の順序保証ポリシー

`ap_parse_known_args(...)` は unknown トークン列を「出現順」で保持します。対象は次を含みます。

- 連続した未知オプション
- 未知オプションと値の交互列
- `--` 以降のトークン列
- 入れ子サブコマンドをまたいで統合される unknown

したがって、forwarding 先には元入力と同じ相対順序で unknown が渡されます。

## メモリ解放

unknown 配列は `ap_free_tokens(...)` で解放します。

```c
ap_free_tokens(unknown, unknown_count);
```

## intermixed 系 (`ap_parse_intermixed_args` / `ap_parse_known_intermixed_args`)

「オプションと位置引数を交互に並べる入力」を API 名で明示したい場合に使います。

## `ap_parse_args` / `ap_parse_known_args` との違い

- `ap_parse_intermixed_args(...)` は現状 `ap_parse_args(...)` のエイリアス動作です。
- `ap_parse_known_intermixed_args(...)` は現状 `ap_parse_known_args(...)` のエイリアス動作です。
- つまり、パース/検証ルールとエラーモデルは同一で、intermixed 名は意味づけ上の別名です。

```c
ap_parse_intermixed_args(parser, argc, argv, &ns, &err);
ap_parse_known_intermixed_args(parser, argc, argv, &ns, &unknown, &unknown_count, &err);
```

根拠テスト:

- intermixed API で混在順序 (`a.txt --count 2 b.txt`) が通る:
  [`ParseIntermixedAliasParsesMixedOrder`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_parse_core.cpp)
- known 側の unknown 回収規則は同一:
  [`ParseKnownArgsCollectsUnknownOptionValueToken`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp),
  [`ParseKnownArgsCollectsAfterDoubleDash`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)

## 典型ユースケース（オプションと位置引数の交互入力）

例:

```text
prog input1 --count 2 input2 --mode fast input3
```

ラッパー CLI やファイル処理系 CLI では、こうした交互入力が自然です。  
現時点の動作は通常 API と同じでも、intermixed API 名を使うことで意図をコード上で明示できます。

## unknown args 回収時の挙動

`ap_parse_known_intermixed_args(...)` の unknown 回収ルールは `ap_parse_known_args(...)` と同一です:

- 未知オプションを回収
- 未知オプション直後の値らしいトークンを回収
- positional 割り当て後に余ったトークンを回収
- `--` 以降のすべてのトークンを回収

参照:

- [`ParseKnownArgsCollectsUnknownOptionValueToken`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- [`ParseKnownArgsCollectsExtraPositionals`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- [`ParseKnownArgsCollectsAfterDoubleDash`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)

## サブコマンド・`nargs` と組み合わせる際の注意点

- サブコマンド処理は通常 API と同一です。  
  known 系では、入れ子サブコマンドの unknown を統合しつつ、`subcommand_path` などのメタ情報を保持します。  
  参照: [`ParseKnownArgsMergesNestedSubcommandUnknownsAndPreservesPath`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- `nargs` の挙動も通常 API と同一です。  
  たとえば optional nargs は既知オプションを飲み込まず、可変長 positional も既存の束縛ルールに従います。  
  参照: [`OptionalNargsDoesNotConsumeFollowingKnownOption`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp),
  [`PositionalZeroOrMoreLeavesTokensForRequiredPositional`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)

## サブコマンドと併用したときの推奨フロー

1. ルート parser で `ap_parse_known_args(...)` を実行して、ラッパー共通オプションを確定する。
2. namespace の `subcommand` / `subcommand_path` で実行先を決める。
3. 回収した unknown を、そのサブコマンド実装（または外部プロセス）へ渡す。
4. プラグイン固有フラグを透過転送する境界には `--` を置く。

この流れにすると、ラッパー側の責務と下流側の責務が明確になり、保守しやすくなります。

## セキュリティ観点（意図しないフラグ注入回避）

外部入力や半信頼入力を受けるラッパーでは、下流引数の前に `--` を挿入して
解釈境界を固定することを推奨します。

- 区切りなしのリスク: たとえば `--verbose` のようなトークンがラッパー既知フラグとして解釈されうる
- 区切りありの緩和: `--` 以降を unknown としてそのまま転送できる

実運用では「forwarding 境界は `--` 必須」という規約化が有効です。
