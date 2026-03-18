#!/usr/bin/env python3
"""
Fix Doc Links
=============
Post-recovery fixup script for the converted KADAS markdown documentation.

Problems addressed:
  1. Image paths like ``../../../../images/foo.png`` or ``/images/foo.png``
     that are leftovers from the original QGIS HTML source structure.
     All affected markdown files sit at depth 2 inside docs/ (e.g.
     en/working_with_vector/vector_properties.md), so the correct relative
     path to docs/images/ is always ``../images/foo.png``.

  2. Internal cross-links still using the ``.html`` extension instead of
     ``.md`` (e.g. ``expression.html#vector-expressions``).

  3. Cross-links pointing to QGIS upstream pages that don't exist in this
     repo (``../introduction/...``, ``../working_with_vector/...`` etc.).
     These are replaced with plain text so the build doesn't warn about them.

  4. Copy the upstream QGIS images from docs_old/html/images/ into
     docs/images/ so the corrected paths actually resolve.

Usage:
    python3 scripts/fix_doc_links.py [--dry-run]
"""

import argparse
import re
import shutil
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT   = SCRIPT_DIR.parent
DOCS_DIR    = REPO_ROOT / "docs"
IMAGES_SRC  = REPO_ROOT / "docs_old" / "html" / "images"
IMAGES_DST  = DOCS_DIR / "images"


# ---------------------------------------------------------------------------
# Regex helpers
# ---------------------------------------------------------------------------

# Matches any number of leading "../" segments followed by "images/filename"
# e.g.  ../../../../images/foo.png
#        ../images/foo.png
#        /images/foo.png   (absolute-style)
# All affected files sit at depth 2 inside docs/ (e.g. en/working_with_vector/file.md),
# so MkDocs resolves relative paths from that position within docs_dir.
# The correct path to docs/images/ from depth-2 is always "../../images/".
RE_IMAGE_PATH = re.compile(
    r'(?:(?:\.\./)*)images/([^)\s\'"]+)'
    r'|'
    r'/images/([^)\s\'"]+)'
)

CORRECT_IMAGE_PREFIX = "../../images/"

# Matches .html cross-links (relative, not http)
# e.g.  expression.html#vector-expressions
#        ../working_with_vector/vector_properties.html#vector-properties-dialog
#        vector_properties.html#vector-style-menu
RE_HTML_LINK = re.compile(
    r'\[([^\]]+)\]\(([^)]*?\.html(?:#[^)]*)?)\)'
)

# Pages that exist in our MkDocs site (relative to the same language subtree)
KNOWN_PAGES = {
    "expression",
    "vector_properties",
    "field_calculator",
    "supported_data",
    "raster_properties",
    "working_with_projections",
    "print_composer",
    "gpsgate",
}


def fix_image_paths(text: str) -> str:
    """Replace any ``(../)*images/`` or ``/images/`` prefix with ``../../images/``."""

    def _replace(m: re.Match) -> str:
        filename = m.group(1) or m.group(2)
        return f"{CORRECT_IMAGE_PREFIX}{filename}"

    return RE_IMAGE_PATH.sub(_replace, text)


def fix_html_links(text: str) -> str:
    """
    Convert .html cross-links to .md, or strip them to plain text if the
    target page doesn't exist in this repo.
    """

    def _replace(m: re.Match) -> str:
        label = m.group(1)
        href  = m.group(2)

        # Extract just the page stem (last path component without extension/anchor)
        path_part = href.split("#")[0]          # drop anchor
        stem      = Path(path_part).stem        # e.g. "expression"

        if stem in KNOWN_PAGES:
            # Keep as a link but use .md extension
            new_href = re.sub(r"\.html", ".md", href)
            return f"[{label}]({new_href})"
        else:
            # Page doesn't exist in our docs – render as plain text
            return label

    return RE_HTML_LINK.sub(_replace, text)


def process_file(path: Path, dry_run: bool) -> int:
    """Apply all fixes to a single markdown file. Returns number of changes."""
    original = path.read_text(encoding="utf-8")

    text = original
    text = fix_image_paths(text)
    text = fix_html_links(text)

    changes = sum(a != b for a, b in zip(original.splitlines(), text.splitlines()))
    changes += abs(len(original.splitlines()) - len(text.splitlines()))

    if text != original:
        if dry_run:
            print(f"  [dry-run] would patch: {path.relative_to(REPO_ROOT)}")
        else:
            path.write_text(text, encoding="utf-8")
            print(f"  patched: {path.relative_to(REPO_ROOT)}")
    return changes


def copy_images(dry_run: bool) -> None:
    """Copy the upstream QGIS images into docs/images/."""
    if not IMAGES_SRC.exists():
        print(
            f"WARNING: image source directory not found: {IMAGES_SRC}\n"
            "         Skipping image copy. Make sure docs_old/ is present.",
            file=sys.stderr,
        )
        return

    if dry_run:
        count = sum(1 for _ in IMAGES_SRC.iterdir() if _.is_file())
        print(f"  [dry-run] would copy {count} images  {IMAGES_SRC} -> {IMAGES_DST}")
        return

    IMAGES_DST.mkdir(parents=True, exist_ok=True)
    count = 0
    for src_file in IMAGES_SRC.iterdir():
        if src_file.is_file():
            dst_file = IMAGES_DST / src_file.name
            if not dst_file.exists():
                shutil.copy2(src_file, dst_file)
                count += 1
    print(f"  copied {count} new image(s) to {IMAGES_DST.relative_to(REPO_ROOT)}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Fix image paths and .html links in recovered markdown docs.")
    parser.add_argument("--dry-run", action="store_true", help="Show what would change without writing files.")
    args = parser.parse_args()

    print("=== Step 1: Copy upstream images ===")
    copy_images(args.dry_run)

    print("\n=== Step 2: Fix markdown files ===")
    md_files = sorted(DOCS_DIR.rglob("*.md"))
    total_files_changed = 0
    for md_file in md_files:
        changes = process_file(md_file, args.dry_run)
        if changes:
            total_files_changed += 1

    print(f"\nDone. {total_files_changed} file(s) {'would be' if args.dry_run else 'were'} modified.")


if __name__ == "__main__":
    main()