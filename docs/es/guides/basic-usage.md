# Guía básica

## Referencia API (traducción planificada)

La referencia completa de API está disponible por ahora en inglés y japonés (traducción futura):
- [API Reference (English)](../../api-spec.en.md)
- [API仕様（日本語）](../../api-spec.ja.md)

[Inicio](../index.md) | [Getting Started](../getting-started.md) | [FAQ](../faq.md)

Si eres nuevo en `argparse-c`, el modelo mental más simple es este flujo de cuatro pasos:

1. Crear un parser con `ap_parser_new(...)`
2. Definir argumentos con `ap_add_argument(...)`
3. Parsear la línea de comandos con `ap_parse_args(...)`
4. Leer resultados con `ap_ns_get_*`

## Argumentos opcionales

```c
ap_arg_options text = ap_arg_options_default();
text.required = true;
text.help = "text value";
ap_add_argument(parser, "-t, --text", text, &err);
```

- acepta tanto `-t` como `--text`
- se vuelve obligatorio cuando `required = true`

## Argumentos posicionales

```c
ap_arg_options input = ap_arg_options_default();
input.help = "input file";
ap_add_argument(parser, "input", input, &err);
```

- los posicionales se declaran sin flags
- aparecen como entradas posicionales en usage/help

## Manejo de errores

`argparse-c` no llama a `exit()` dentro de la librería. En caso de fallo, revisa `ap_error` y opcionalmente renderiza un mensaje para usuarios con `ap_format_error(...)`.

```c
if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
  char *message = ap_format_error(parser, &err);
  fprintf(stderr, "%s", message ? message : err.message);
  free(message);
}
```

## Salida de ayuda

`-h/--help` se agrega automáticamente.

```c
char *help = ap_format_help(parser);
printf("%s", help);
free(help);
```
