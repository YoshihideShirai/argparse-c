# Getting Started

## Referencia API (traducción planificada)

La referencia completa de API está disponible por ahora en inglés y japonés (traducción futura):
- [API Reference (English)](../api-spec.en.md)
- [API仕様（日本語）](../api-spec.ja.md)

[Inicio](index.md) | [Guía básica](guides/basic-usage.md) | [FAQ](faq.md)

Esta página es la ruta principal de incorporación desde el README del repositorio. Explica **cómo instalar `argparse-c`, ejecutar el primer ejemplo y elegir la siguiente guía**.

## Requisitos previos

- C99 compiler
- CMake
- Git

## Compilar e instalar

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
cmake --install build --prefix /usr/local
```

## Programa de ejemplo

The repository includes `sample/example1.c`.

## Ejecución de ejemplo

```bash
./build/sample/example1 -t hello -i 42 input.txt extra.txt
```

Expected output:

```text
text=hello
integer=42
arg1=input.txt
arg2=extra.txt
```

## Primeras APIs que debes aprender

### 1. Create a parser

```c
ap_parser *parser = ap_parser_new("demo", "demo parser");
```

### 2. Define arguments

```c
ap_arg_options opts = ap_arg_options_default();
opts.required = true;
opts.help = "input text";
ap_add_argument(parser, "-t, --text", opts, &err);
```

### 3. Parse the command line

```c
ap_parse_args(parser, argc, argv, &ns, &err);
```

### 4. Read values from the namespace

```c
const char *text = NULL;
ap_ns_get_string(ns, "text", &text);
```

## Siguiente lectura recomendada

- [Basic Guide](guides/basic-usage.md)
- [FAQ](faq.md)
- [Home](index.md)
