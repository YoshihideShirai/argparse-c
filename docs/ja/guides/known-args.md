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
  [`ParseIntermixedAliasParsesMixedOrder`](../../../test/test_parse_core.cpp)
- known 側の unknown 回収規則は同一:
  [`ParseKnownArgsCollectsUnknownOptionValueToken`](../../../test/test_known_args.cpp),
  [`ParseKnownArgsCollectsAfterDoubleDash`](../../../test/test_known_args.cpp)

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

- [`ParseKnownArgsCollectsUnknownOptionValueToken`](../../../test/test_known_args.cpp)
- [`ParseKnownArgsCollectsExtraPositionals`](../../../test/test_known_args.cpp)
- [`ParseKnownArgsCollectsAfterDoubleDash`](../../../test/test_known_args.cpp)

## サブコマンド・`nargs` と組み合わせる際の注意点

- サブコマンド処理は通常 API と同一です。  
  known 系では、入れ子サブコマンドの unknown を統合しつつ、`subcommand_path` などのメタ情報を保持します。  
  参照: [`ParseKnownArgsMergesNestedSubcommandUnknownsAndPreservesPath`](../../../test/test_known_args.cpp)
- `nargs` の挙動も通常 API と同一です。  
  たとえば optional nargs は既知オプションを飲み込まず、可変長 positional も既存の束縛ルールに従います。  
  参照: [`OptionalNargsDoesNotConsumeFollowingKnownOption`](../../../test/test_known_args.cpp),
  [`PositionalZeroOrMoreLeavesTokensForRequiredPositional`](../../../test/test_known_args.cpp)
