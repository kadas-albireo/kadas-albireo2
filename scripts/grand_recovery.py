#!/usr/bin/env python3
"""
Grand Recovery Script
=====================
Extracts content from legacy KADAS HTML documentation and converts it to
clean Markdown files ready for MkDocs.

Phase 1 (deterministic): BeautifulSoup extraction + markdownify conversion
Phase 2 (structural):    Fix image paths, anchor IDs, and generate mkdocs.yml nav skeleton

Usage:
    # Single file (prototype/debug mode):
    python3 grand_recovery.py --file share/docs/html/en/en/analysis/index.html

    # Full recovery, all languages:
    python3 grand_recovery.py --all

    # Specific language only:
    python3 grand_recovery.py --lang en

Output:
    docs/
    ├── en/
    │   ├── analysis.md
    │   ├── draw.md
    │   └── ...
    ├── de/
    ├── fr/
    ├── it/
    └── media/       ← shared images copied here
    mkdocs.nav.yml   ← nav skeleton to paste into mkdocs.yml
"""

import argparse
import re
import shutil
import sys
from pathlib import Path

from bs4 import BeautifulSoup
from markdownify import MarkdownConverter

# ---------------------------------------------------------------------------
# Paths (all relative to the repo root, i.e. the parent of this script)
# ---------------------------------------------------------------------------
SCRIPT_DIR   = Path(__file__).resolve().parent
REPO_ROOT    = SCRIPT_DIR.parent
HTML_ROOT    = REPO_ROOT / "share" / "docs" / "html"
MEDIA_SRC    = HTML_ROOT / "media"
OUTPUT_ROOT  = REPO_ROOT / "docs"
MEDIA_DST    = OUTPUT_ROOT / "media"

LANGUAGES    = ["en", "de", "fr", "it"]

# Human-readable section titles extracted from the nav in the HTML.
# Used to build the mkdocs.yml nav skeleton.  The key is the folder name.
SECTION_TITLES: dict[str, str] = {
    "index":                        "General",
    "kadas_gui":                    "The Interface",
    "map":                          "Map",
    "view":                         "View",
    "analysis":                     "Analysis",
    "draw":                         "Draw",
    "gps":                          "Navigation",
    "mss":                          "MSS",
    "settings":                     "Settings",
    "working_with_vector":          "Advanced / Vector Layers",
    "working_with_raster":          "Advanced / Raster Layers",
    "working_with_projections":     "Advanced / Working with Projections",
    "print_composer":               "Advanced / Print Composer",
    "gpsgate":                      "Advanced / GPS Gate",
    "appendices":                   "Appendix",
}


# ---------------------------------------------------------------------------
# Custom markdownify converter
# ---------------------------------------------------------------------------
class KadasConverter(MarkdownConverter):
    """
    Extends the default MarkdownConverter with KADAS-specific tweaks:

    - Drops <a name="secN"> anchor tags (legacy HTML anchors — MkDocs
      generates its own heading anchors from the heading text).
    - Converts <strong><em>…</em></strong> (UI element names in the source)
      to plain **…** bold, stripping the redundant italic wrapper.
    - Normalises whitespace in the output so we never get more than one
      blank line between blocks.
    """

    def convert_a(self, el, text, parent_tags):
        # Drop legacy <a name="secN"> anchors — keep nothing, not even the text
        # because these anchors are always *inside* a heading tag and the
        # heading text is already captured by the heading converter.
        if el.get("name") and not el.get("href"):
            return ""
        return super().convert_a(el, text, parent_tags)

    def convert_img(self, el, text, parent_tags):
        # BeautifulSoup gives us the src as-is from the HTML.
        # We fix the path separately after conversion; here we just pass through.
        return super().convert_img(el, text, parent_tags)


def html_to_markdown(html_fragment: str) -> str:
    """Convert an HTML string to Markdown using the custom converter."""
    md = KadasConverter(
        heading_style="ATX",      # Use # style headings
        bullets="-",              # Use - for unordered lists
        strong_em_symbol="**",
        newline_style="backslash",
    ).convert(html_fragment)

    # Collapse ****** (strong+em double-wrap) into plain ** bold.
    # markdownify renders <strong><em>text</em></strong> as ******text******
    # but in KADAS docs this pattern is purely used for UI element names,
    # so plain bold is the right semantic and much cleaner to read/edit.
    md = re.sub(r"\*{3,}(.+?)\*{3,}", r"**\1**", md)

    # Fix nested list indentation: markdownify emits 2-space indented nested
    # list items, but Python-Markdown (used by MkDocs) requires 4 spaces to
    # recognise nesting. We expand every run of leading 2-space pairs into
    # 4-space pairs, but only on lines that are list items (start with spaces
    # followed by "- " or "N. ").
    def fix_list_indent(line: str) -> str:
        stripped = line.lstrip(" ")
        n_spaces = len(line) - len(stripped)
        if n_spaces == 0:
            return line
        # Only reindent actual list item lines
        if not (stripped.startswith("- ") or (stripped[:1].isdigit() and ". " in stripped[:4])):
            return line
        # Each level of 2-space indent becomes 4-space indent
        levels = n_spaces // 2
        remainder = n_spaces % 2
        return " " * (levels * 4 + remainder) + stripped

    md = "\n".join(fix_list_indent(line) for line in md.splitlines())

    # Remove blank lines between a parent list item and its indented children.
    # Python-Markdown breaks nesting when a loose parent item (followed by a
    # blank line) precedes indented children — it treats the children as a new
    # top-level block instead of a nested list.
    # Pattern: "- text\n\n    - child"  →  "- text\n    - child"
    md = re.sub(r"(\n- .+)\n\n(    - )", r"\1\n\2", md)

    # Collapse 3+ consecutive blank lines into exactly one blank line
    md = re.sub(r"\n{3,}", "\n\n", md)
    return md.strip()


# ---------------------------------------------------------------------------
# Image path rewriting
# ---------------------------------------------------------------------------
def rewrite_image_paths(md: str, section: str, lang: str) -> str:
    """
    In the source HTML, images are referenced as ../../../media/imageN.png
    relative to docs/{lang}/{lang}/{section}/index.html.

    In the new structure they will live at docs/{lang}/{section}.md and
    the shared media folder will be at docs/media/.  MkDocs resolves paths
    relative to the .md file, so the correct relative path is:
        ../media/imageN.png

    We also handle the top-level index (section == "index") whose file will
    be at docs/{lang}/index.md — same depth, same relative path.
    """
    # Match both ../../../media/... and ../../media/... (defensive)
    pattern = re.compile(r"\.\./(?:\.\./)*media/([\w.\-]+)")
    replacement = r"../media/\1"
    return pattern.sub(replacement, md)


# ---------------------------------------------------------------------------
# Core extraction logic
# ---------------------------------------------------------------------------
def extract_content_div(html_path: Path) -> BeautifulSoup | None:
    """
    Parse the HTML file and return the articleBody div as a BeautifulSoup
    object, or None if the expected structure is not found.
    """
    text = html_path.read_text(encoding="utf-8")
    soup = BeautifulSoup(text, "html.parser")
    content = soup.find("div", {"itemprop": "articleBody"})
    if content is None:
        print(f"  [WARN] No articleBody div found in {html_path}", file=sys.stderr)
    return content


def recover_page(html_path: Path, lang: str, section: str) -> str:
    """
    Full pipeline for a single page:
      1. Extract the content div
      2. Convert to Markdown
      3. Rewrite image paths
      4. Return the final Markdown string
    """
    content_div = extract_content_div(html_path)
    if content_div is None:
        return f"<!-- Recovery failed for {html_path} -->\n"

    raw_md = html_to_markdown(str(content_div))
    fixed_md = rewrite_image_paths(raw_md, section, lang)

    # Add a metadata comment header so we can trace provenance
    header = (
        f"<!-- Recovered from: {html_path.relative_to(REPO_ROOT)} -->\n"
        f"<!-- Language: {lang} | Section: {section} -->\n\n"
    )
    return header + fixed_md + "\n"


# ---------------------------------------------------------------------------
# File discovery
# ---------------------------------------------------------------------------
def discover_sections(lang: str) -> list[tuple[str, Path]]:
    """
    Walk the {lang}/{lang}/ subtree and return a list of (section_name, html_path)
    tuples, sorted so that the top-level index comes first.

    A "section" is either:
      - The lang dir itself (docs/{lang}/{lang}/index.html  →  "index")
      - Any subfolder containing an index.html
    """
    lang_dir = HTML_ROOT / lang / lang
    if not lang_dir.is_dir():
        print(f"  [WARN] Language directory not found: {lang_dir}", file=sys.stderr)
        return []

    sections: list[tuple[str, Path]] = []

    # Top-level index.html for this language
    top_index = lang_dir / "index.html"
    if top_index.exists():
        sections.append(("index", top_index))

    # Sub-section index.html files
    for sub in sorted(lang_dir.iterdir()):
        if sub.is_dir():
            sub_index = sub / "index.html"
            if sub_index.exists():
                sections.append((sub.name, sub_index))

    return sections


# ---------------------------------------------------------------------------
# Media copy
# ---------------------------------------------------------------------------
def copy_media() -> None:
    """Copy the shared media/ folder to docs/media/ (skip if already up to date)."""
    if not MEDIA_SRC.is_dir():
        print(f"  [WARN] Media source not found: {MEDIA_SRC}", file=sys.stderr)
        return

    MEDIA_DST.mkdir(parents=True, exist_ok=True)
    copied = 0
    for src_file in MEDIA_SRC.iterdir():
        dst_file = MEDIA_DST / src_file.name
        if not dst_file.exists() or src_file.stat().st_mtime > dst_file.stat().st_mtime:
            shutil.copy2(src_file, dst_file)
            copied += 1

    print(f"  [media] {copied} file(s) copied/updated → {MEDIA_DST.relative_to(REPO_ROOT)}")


# ---------------------------------------------------------------------------
# mkdocs.yml nav skeleton generator
# ---------------------------------------------------------------------------
def generate_nav_skeleton(recovered: dict[str, list[str]]) -> str:
    """
    Build a YAML nav block from the recovered sections.

    recovered = { "en": ["index", "kadas_gui", ...], "de": [...], ... }
    """
    lines = ["# Paste this nav block into your mkdocs.yml", "nav:"]

    for lang in LANGUAGES:
        if lang not in recovered:
            continue
        lines.append(f"  # --- {lang.upper()} ---")
        for section in recovered[lang]:
            title = SECTION_TITLES.get(section, section.replace("_", " ").title())
            md_path = f"{lang}/{section}.md"
            lines.append(f"  - '{title}': '{md_path}'")

    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# Recovery runners
# ---------------------------------------------------------------------------
def run_single_file(html_path: Path) -> None:
    """Prototype mode: recover a single file and print to stdout."""
    html_path = html_path.resolve()

    # Infer lang and section from the path structure
    # Expected: .../html/{lang}/{lang}/{section}/index.html
    #       or: .../html/{lang}/{lang}/index.html
    parts = html_path.parts
    try:
        html_idx = parts.index("html")
        lang    = parts[html_idx + 1]
        # section is either the folder name or "index" for the top-level file
        if parts[html_idx + 3] == "index.html":
            section = "index"
        else:
            section = parts[html_idx + 3]
    except (ValueError, IndexError):
        lang    = "unknown"
        section = html_path.parent.name or "unknown"

    print(f"[prototype] Recovering: lang={lang}, section={section}")
    print(f"            Source:     {html_path}")
    print("-" * 72)
    md = recover_page(html_path, lang, section)
    print(md)


def run_language(lang: str, dry_run: bool = False) -> list[str]:
    """Recover all sections for a single language. Returns list of section names."""
    print(f"\n[{lang}] Starting recovery...")
    sections = discover_sections(lang)
    if not sections:
        return []

    lang_out = OUTPUT_ROOT / lang
    if not dry_run:
        lang_out.mkdir(parents=True, exist_ok=True)

    recovered_sections = []
    for section, html_path in sections:
        out_file = lang_out / f"{section}.md"
        print(f"  [{lang}/{section}] {html_path.relative_to(REPO_ROOT)} → {out_file.relative_to(REPO_ROOT)}")

        md = recover_page(html_path, lang, section)

        if not dry_run:
            out_file.write_text(md, encoding="utf-8")
            print(f"    ✓ Written ({len(md)} chars)")
        else:
            print(f"    [dry-run] Would write {len(md)} chars")

        recovered_sections.append(section)

    return recovered_sections


def run_all(dry_run: bool = False) -> None:
    """Recover all languages and copy media."""
    print("=" * 72)
    print("KADAS Grand Recovery")
    print("=" * 72)

    if not dry_run:
        copy_media()

    recovered: dict[str, list[str]] = {}
    for lang in LANGUAGES:
        sections = run_language(lang, dry_run=dry_run)
        if sections:
            recovered[lang] = sections

    # Write the nav skeleton
    nav_yaml = generate_nav_skeleton(recovered)
    nav_path = REPO_ROOT / "mkdocs.nav.yml"
    if not dry_run:
        nav_path.write_text(nav_yaml, encoding="utf-8")
        print(f"\n[nav] Written → {nav_path.relative_to(REPO_ROOT)}")
    else:
        print(f"\n[nav] [dry-run] Would write → {nav_path.relative_to(REPO_ROOT)}")

    total = sum(len(v) for v in recovered.values())
    print(f"\n{'=' * 72}")
    print(f"Done. {total} page(s) recovered across {len(recovered)} language(s).")
    if not dry_run:
        print(f"Output: {OUTPUT_ROOT.relative_to(REPO_ROOT)}/")
    print("=" * 72)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="KADAS Grand Recovery — HTML to Markdown conversion tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    mode = p.add_mutually_exclusive_group(required=True)
    mode.add_argument(
        "--file", "-f",
        metavar="PATH",
        type=Path,
        help="Prototype mode: recover a single HTML file and print to stdout.",
    )
    mode.add_argument(
        "--lang", "-l",
        metavar="LANG",
        choices=LANGUAGES,
        help="Recover all pages for one language (en/de/fr/it).",
    )
    mode.add_argument(
        "--all", "-a",
        action="store_true",
        help="Full recovery: all languages + media copy + nav skeleton.",
    )

    p.add_argument(
        "--dry-run", "-n",
        action="store_true",
        help="Show what would be done without writing any files.",
    )

    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    if args.file:
        run_single_file(args.file)
    elif args.lang:
        run_language(args.lang, dry_run=args.dry_run)
        if not args.dry_run:
            copy_media()
    elif args.all:
        run_all(dry_run=args.dry_run)


if __name__ == "__main__":
    main()