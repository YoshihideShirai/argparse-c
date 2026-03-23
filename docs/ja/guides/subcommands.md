# サブコマンド

`argparse-c` は nested subcommands をサポートしています。

## 追加方法

```c
ap_parser *config = ap_add_subcommand(parser, "config", "config commands", &err);
ap_parser *set = ap_add_subcommand(config, "set", "set a value", &err);
```

## パース結果

パース成功時、namespace の組み込みキーとして `"subcommand"` と `"subcommand_path"` が使えます。

- `subcommand` には **最終的に選ばれた leaf subcommand 名** が入ります
- 中間階層は別キーとして保存されません
- `subcommand_path` には選択されたサブコマンド列全体が入ります

## 例

```c
const char *subcommand = NULL;
const char *subcommand_path = NULL;
ap_ns_get_string(ns, "subcommand", &subcommand);
ap_ns_get_string(ns, "subcommand_path", &subcommand_path);
```

```bash
prog config set theme dark
```

この場合、`subcommand` には `set`、`subcommand_path` には `config set` が入ります。

## help 表示

nested subcommand の help はフルコマンドパスで表示されます。

例:

```text
usage: prog config set
```
