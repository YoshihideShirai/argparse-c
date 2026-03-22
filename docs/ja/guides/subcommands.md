# サブコマンド

`argparse-c` は nested subcommands をサポートしています。

## 追加方法

```c
ap_parser *config = ap_add_subcommand(parser, "config", "config commands", &err);
ap_parser *set = ap_add_subcommand(config, "set", "set a value", &err);
```

## パース結果

パース成功時、namespace の組み込みキーとして `"subcommand"` が使えます。

- 保存されるのは **最終的に選ばれた leaf subcommand 名**
- 中間階層は別キーとして保存されません
- `subcommand_path` は現在の公開 contract には含まれません

## 例

```c
const char *subcommand = NULL;
ap_ns_get_string(ns, "subcommand", &subcommand);
```

```bash
prog config set theme dark
```

この場合、`subcommand` には `set` が入ります。

## help 表示

nested subcommand の help はフルコマンドパスで表示されます。

例:

```text
usage: prog config set
```
