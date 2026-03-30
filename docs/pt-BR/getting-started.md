# Getting Started

## Referência de API (tradução planejada)

A referência completa de API está disponível, por enquanto, em inglês e japonês (tradução futura):
- [API Reference (English)](../api-spec.en.md)
- [API仕様（日本語）](../api-spec.ja.md)

[Início](index.md) | [Guia básico](guides/basic-usage.md) | [FAQ](faq.md)

Esta página é o caminho principal de onboarding a partir do README do repositório. Ela mostra **como instalar `argparse-c`, executar o primeiro exemplo e escolher o próximo guia**.

## Pré-requisitos

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

## Programa de exemplo

The repository includes `sample/example1.c`.

## Execução de exemplo

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

## Primeiras APIs para aprender

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

## Próxima leitura recomendada

- [Basic Guide](guides/basic-usage.md)
- [FAQ](faq.md)
- [Home](index.md)
