#!/usr/bin/env python3
"""Build documentation site with Zensical."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


def main() -> int:
    zensical = shutil.which("zensical")
    if zensical is None:
        print("zensical command not found", file=sys.stderr)
        return 1

    command = [zensical, "build"]
    print("+", " ".join(command), flush=True)
    subprocess.run(command, check=True)

    site_dir = Path("site")
    if not site_dir.exists():
        print("site directory does not exist after zensical build", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
