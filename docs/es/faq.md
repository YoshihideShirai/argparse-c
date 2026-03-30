# FAQ

## Referencia API (traducción planificada)

La referencia completa de API está disponible por ahora en inglés y japonés (traducción futura):
- [API Reference (English)](../api-spec.en.md)
- [API仕様（日本語）](../api-spec.ja.md)

[Inicio](index.md) | [Getting Started](getting-started.md) | [Guía básica](guides/basic-usage.md)

## ¿`argparse-c` termina el proceso en errores de parseo?

No. Las APIs de parseo devuelven `0` en éxito y `-1` en fallo, y el detalle se escribe en `ap_error`.

## ¿Cómo imprimo la ayuda?

`-h/--help` se agrega automáticamente. Si necesitas el texto formateado, llama a `ap_format_help(...)`.

## ¿Cómo se decide `dest`?

- los argumentos opcionales priorizan la primera bandera larga
- si no hay bandera larga, se usa la primera bandera corta
- los posicionales usan el nombre declarado
- durante la autogeneración, `-` se normaliza a `_`

## ¿Dónde está la referencia completa de API?

- Japanese: [API Reference（日本語）](../api-spec.ja.md)
- English: [API Reference (English)](../api-spec.en.md)
