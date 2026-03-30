# FAQ

## Referência de API (tradução planejada)

A referência completa de API está disponível, por enquanto, em inglês e japonês (tradução futura):
- [API Reference (English)](../api-spec.en.md)
- [API仕様（日本語）](../api-spec.ja.md)

[Início](index.md) | [Getting Started](getting-started.md) | [Guia básico](guides/basic-usage.md)

## O `argparse-c` encerra o processo em erros de parse?

Não. As APIs de parse retornam `0` em sucesso e `-1` em falha, e os detalhes ficam em `ap_error`.

## Como imprimo ajuda?

`-h/--help` é adicionado automaticamente. Se você quiser o texto formatado, chame `ap_format_help(...)`.

## Como `dest` é escolhido?

- argumentos opcionais priorizam a primeira flag longa
- se não houver flag longa, usa a primeira flag curta
- posicionais usam o nome declarado
- durante a autogeração, `-` é normalizado para `_`

## Onde está a referência completa de API?

- Japanese: [API Reference（日本語）](../api-spec.ja.md)
- English: [API Reference (English)](../api-spec.en.md)
