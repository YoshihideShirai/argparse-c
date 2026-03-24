# FAQ

## Does `argparse-c` exit on parse errors?

No. Parse APIs return `0` on success and `-1` on failure, and the details are written into `ap_error`.

## How do I print help?

`-h/--help` is added automatically. If you want the formatted help text, call `ap_format_help(...)`.

## How is `dest` chosen?

- optional arguments prefer the first long flag
- if there is no long flag, the first short flag is used
- positionals use the declared name
- during auto-generation, `-` is normalized to `_`

## Where is the full API reference?

- Japanese: [API Reference（日本語）](../api-spec.ja.md)
- English: [API Reference (English)](../api-spec.en.md)

## What is the current security validation statement?

- We continuously run sanitizer and boundary-focused tests in CI and publish coverage:
  - CI workflow (tests, sanitizers, coverage): <https://github.com/yoshihideshirai/argparse-c/actions/workflows/ci.yml>
  - Pages workflow (coverage publish): <https://github.com/yoshihideshirai/argparse-c/actions/workflows/pages.yml>
- Security test procedure: [Security testing procedure](./security-testing.md)
- Known status at publish time: no known unpatched critical vulnerabilities at publication time.

### What is out of guarantee scope?

- Input/output validation in applications using `argparse-c` is the responsibility of each application.
- Behavioral/security impact from environment differences (OS, toolchain, dependency versions) is outside the library's full guarantee scope.

### How often is this statement updated?

- The wording is reviewed and updated at each release, based on current CI evidence and the security-test procedure.
