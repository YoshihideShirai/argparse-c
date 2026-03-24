# AGENTS.md

## Working rules
- Before finishing any code change in this repository, always run the project formatter.
- Preferred formatter command: `cmake --build build --target format`
- If the `build/` directory does not exist yet, create it first with `cmake -S . -B build`, then run the formatter target.
- After formatting, rerun any relevant build/tests needed for the change.
- If you modify CI workflow files (for example `.github/workflows/*.yml`), you must run and report relevant local checks that validate the CI-targeted behavior before finishing.
