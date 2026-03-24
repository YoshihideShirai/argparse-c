#!/usr/bin/env python3
"""Validate docs source links and generated site links for GitHub Pages safety."""

from __future__ import annotations

import argparse
import html.parser
import re
import sys
from pathlib import Path
from urllib.parse import urlparse

FORBIDDEN_SOURCE_PATTERNS = (
    re.compile(r"\(\.\./README"),
    re.compile(r"\(\.\./\.\./sample/"),
)


class HrefCollector(html.parser.HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.hrefs: list[tuple[int, str]] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        if tag != "a":
            return
        for name, value in attrs:
            if name == "href" and value:
                self.hrefs.append((self.getpos()[0], value))


def validate_docs_source(docs_dir: Path) -> list[str]:
    errors: list[str] = []
    for md in sorted(docs_dir.rglob("*.md")):
        text = md.read_text(encoding="utf-8")
        for pattern in FORBIDDEN_SOURCE_PATTERNS:
            for match in pattern.finditer(text):
                line = text.count("\n", 0, match.start()) + 1
                errors.append(
                    f"{md}:{line}: forbidden out-of-site relative link matched {pattern.pattern!r}"
                )
    return errors


def iter_site_html_links(site_dir: Path) -> list[tuple[Path, int, str]]:
    links: list[tuple[Path, int, str]] = []
    for html_file in sorted(site_dir.rglob("*.html")):
        parser = HrefCollector()
        parser.feed(html_file.read_text(encoding="utf-8"))
        for line, href in parser.hrefs:
            links.append((html_file, line, href))
    return links


def link_target_exists(site_dir: Path, source_html: Path, href: str) -> bool:
    parsed = urlparse(href)
    if parsed.scheme or href.startswith("#"):
        return True
    if href.startswith(("mailto:", "javascript:", "tel:")):
        return True

    raw_path = parsed.path
    if not raw_path:
        return True

    if raw_path.startswith("/"):
        target = site_dir / raw_path.lstrip("/")
    else:
        target = source_html.parent / raw_path

    candidates = [target]
    if target.suffix == "":
        candidates.append(target.with_suffix(".html"))
        candidates.append(target / "index.html")
    elif target.name == "index.html":
        candidates.append(target.parent)

    return any(candidate.exists() for candidate in candidates)


def validate_site_links(site_dir: Path) -> list[str]:
    errors: list[str] = []
    for html_file, line, href in iter_site_html_links(site_dir):
        if not link_target_exists(site_dir, html_file, href):
            errors.append(f"{html_file}:{line}: broken link: {href}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--docs-dir", type=Path, default=Path("docs"))
    parser.add_argument("--site-dir", type=Path, default=Path("site"))
    parser.add_argument("--skip-source", action="store_true")
    parser.add_argument("--skip-site", action="store_true")
    args = parser.parse_args()

    errors: list[str] = []

    if not args.skip_source:
        errors.extend(validate_docs_source(args.docs_dir))

    if not args.skip_site:
        if not args.site_dir.exists():
            errors.append(
                f"site directory does not exist: {args.site_dir} (run mkdocs build first)"
            )
        else:
            errors.extend(validate_site_links(args.site_dir))

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    print("docs link checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
