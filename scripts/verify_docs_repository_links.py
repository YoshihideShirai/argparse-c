#!/usr/bin/env python3
"""Run docs/repository synchronization and generated-site link checks in one flow."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


COMMANDS: tuple[tuple[str, ...], ...] = (
    (sys.executable, "scripts/sync_docs_repository.py"),
    (sys.executable, "-m", "mkdocs", "build"),
    (sys.executable, "scripts/check_docs_links.py", "--skip-source"),
)


def run_command(command: tuple[str, ...]) -> None:
    print("+", " ".join(command), flush=True)
    subprocess.run(command, check=True)


def ensure_coverage_placeholder(site_dir: Path) -> None:
    coverage_index = site_dir / "coverage" / "index.html"
    if coverage_index.exists():
        return
    coverage_index.parent.mkdir(parents=True, exist_ok=True)
    coverage_index.write_text(
        "<!doctype html><html><body><p>coverage report is generated in CI.</p></body></html>\n",
        encoding="utf-8",
    )


def main() -> int:
    run_command(COMMANDS[0])
    run_command(COMMANDS[1])
    ensure_coverage_placeholder(Path("site"))
    run_command(COMMANDS[2])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
