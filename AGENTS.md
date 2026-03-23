# AGENTS.md

## Working rules
- Before finishing any code change in this repository, always run the project formatter.
- Preferred formatter command: `cmake --build build --target format`
- If the `build/` directory does not exist yet, create it first with `cmake -S . -B build`, then run the formatter target.
- After formatting, rerun any relevant build/tests needed for the change.
