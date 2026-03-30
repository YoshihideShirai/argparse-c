# Guia básico

## Referência de API (tradução planejada)

A referência completa de API está disponível, por enquanto, em inglês e japonês (tradução futura):
- [API Reference (English)](../../api-spec.en.md)
- [API仕様（日本語）](../../api-spec.ja.md)

[Início](../index.md) | [Getting Started](../getting-started.md) | [FAQ](../faq.md)

Se você é novo em `argparse-c`, o modelo mental mais simples é este fluxo de quatro passos:

1. Criar um parser com `ap_parser_new(...)`
2. Definir argumentos com `ap_add_argument(...)`
3. Fazer parse da linha de comando com `ap_parse_args(...)`
4. Ler resultados com `ap_ns_get_*`

## Argumentos opcionais

```c
ap_arg_options text = ap_arg_options_default();
text.required = true;
text.help = "text value";
ap_add_argument(parser, "-t, --text", text, &err);
```

- aceita tanto `-t` quanto `--text`
- torna-se obrigatório quando `required = true`

## Argumentos posicionais

```c
ap_arg_options input = ap_arg_options_default();
input.help = "input file";
ap_add_argument(parser, "input", input, &err);
```

- posicionais são declarados sem flags
- aparecem como entradas posicionais em usage/help

## Tratamento de erro

`argparse-c` não chama `exit()` dentro da biblioteca. Em caso de falha, inspecione `ap_error` e opcionalmente gere uma mensagem para usuário com `ap_format_error(...)`.

```c
if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
  char *message = ap_format_error(parser, &err);
  fprintf(stderr, "%s", message ? message : err.message);
  free(message);
}
```

## Saída de ajuda

`-h/--help` é adicionado automaticamente.

```c
char *help = ap_format_help(parser);
printf("%s", help);
free(help);
```
