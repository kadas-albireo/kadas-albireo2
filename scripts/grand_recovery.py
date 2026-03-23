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
    ├── media/       ← KADAS screenshots copied here
    └── images/      ← QGIS upstream icons/images copied here
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
HTML_ROOT    = REPO_ROOT / "docs_old" / "html"
MEDIA_SRC    = HTML_ROOT / "media"
IMAGES_SRC   = HTML_ROOT / "images"
OUTPUT_ROOT  = REPO_ROOT / "docs"
MEDIA_DST    = OUTPUT_ROOT / "media"
IMAGES_DST   = OUTPUT_ROOT / "images"

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
    "working_with_vector/supported_data":   "Supported Formats",
    "working_with_vector/vector_properties":"Properties Dialog",
    "working_with_vector/expression":       "Expressions",
    "working_with_vector/field_calculator": "Field Calculator",
    "working_with_raster/supported_data":   "Supported Formats",
    "working_with_raster/raster_properties":"Properties Dialog",
    "working_with_projections/working_with_projections": "Working with Projections",
    "print_composer/print_composer":        "Print Composer",
    "gpsgate/gpsgate":                      "GPS Gate",
    "appendices/shortcuts":                 "Keyboard Shortcuts",
    "appendices/license":                   "License",
}

# Sections that are grouped under a nav parent heading.
# key = section key prefix, value = nav group label
SECTION_GROUPS: dict[str, str] = {
    "working_with_vector":      "Advanced / Vector Layers",
    "working_with_raster":      "Advanced / Raster Layers",
    "working_with_projections": "Advanced / Projections",
    "print_composer":           "Advanced / Print Composer",
    "gpsgate":                  "Advanced / GPS Gate",
    "appendices":               "Appendix",
}


# ---------------------------------------------------------------------------
# Custom markdownify converter
# ---------------------------------------------------------------------------
class KadasConverter(MarkdownConverter):
    """
    Extends the default MarkdownConverter with KADAS-specific tweaks:

    - Drops <a name="secN"> anchor tags (legacy HTML anchors — MkDocs
      generates its own heading anchors from the heading text).
    - Uses _ for <em> (italic) and ** for <strong> (bold) so that
      <strong><em>…</em></strong> renders as ***bold+italic*** and plain
      <em>…</em> renders as _italic_ rather than **bold**.
    - Normalises whitespace in the output so we never get more than one
      blank line between blocks.
    """

    # Override em to use _ so it stays visually distinct from ** bold.
    # Without this, markdownify uses strong_em_symbol ("**") for both
    # <em> and <strong>, making <em>text</em> render as **text** (bold).
    def convert_em(self, el, text, parent_tags):
        from markdownify import chomp
        if "_noformat" in parent_tags:
            return text
        prefix, suffix, text = chomp(text)
        if not text:
            return ""
        return f"{prefix}_{text}_{suffix}"


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
        strong_em_symbol="*",     # convert_b multiplies this by 2 → **, em is overridden to _
        newline_style="backslash",
    ).convert(html_fragment)

    # <strong><em>…</em></strong> now renders as **_text_** — collapse to plain
    # **text** since in KADAS docs this pattern marks UI element names where
    # bold alone is the right semantic and cleaner to read/edit.
    md = re.sub(r"\*\*_(.+?)_\*\*", r"**\1**", md)

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
    Rewrites all image paths to be correct relative to the output .md file.

    Two image pools exist:
      - media/   KADAS screenshots  (../../../media/imageN.png in source HTML)
      - images/  QGIS upstream icons (../../../../images/foo.png or /images/foo.png)

    Output files sit at one of two depths inside docs/:
      - depth 1: docs/{lang}/{section}.md          (flat sections)
      - depth 2: docs/{lang}/{group}/{page}.md     (deep sections)

    Correct relative prefixes:
                    depth-1     depth-2
      media/        ../media/   ../../media/
      images/       ../images/  ../../images/
    """
    depth = section.count("/")  # 0 → depth-1 file, 1 → depth-2 file
    up = "../" * (depth + 1)

    # media/ references (KADAS screenshots)
    md = re.sub(r"\.\./(?:\.\./)*media/([\w.\-]+)", f"{up}media/\\1", md)

    # images/ references (QGIS icons) — relative (one or more ../) and
    # absolute-style (/images/filename).
    # We require at least one ../ for the relative case to avoid matching
    # an already-rewritten path (which would cause double-substitution).
    md = re.sub(r"(?:\.\./)+(images/[\w.\-]+)", f"{up}\\1", md)
    md = re.sub(r"\(/images/([\w.\-]+)", f"({up}images/\\1", md)

    return md


# Pages that exist in our MkDocs site as deep pages (stem only, no extension).
# Used to decide whether an .html cross-link should be kept (converted to .md)
# or stripped to plain text (target doesn't exist in this repo).
_KNOWN_DEEP_PAGES = {
    "expression",
    "vector_properties",
    "field_calculator",
    "supported_data",
    "raster_properties",
    "working_with_projections",
    "print_composer",
    "gpsgate",
}


def rewrite_html_links(md: str) -> str:
    """
    Convert relative .html cross-links to .md, or strip to plain text when
    the target page doesn't exist in this repo.

    Examples:
      [Expressions](expression.html#anchor)  →  [Expressions](expression.md#anchor)
      [Intro](../introduction/intro.html)    →  Intro
    """
    def _replace(m: re.Match) -> str:
        label = m.group(1)
        href  = m.group(2)
        stem  = Path(href.split("#")[0]).stem
        if stem in _KNOWN_DEEP_PAGES:
            return f"[{label}]({re.sub(r'.html', '.md', href)})"
        else:
            return label  # strip broken link, keep readable text

    return re.sub(r'\[([^\]]+)\]\(([^)]*?\.html(?:#[^)]*)?)\)', _replace, md)


def rewrite_internal_links(md: str, section: str) -> str:
    """
    The source HTML used root-relative links like /map, /view, /analysis, etc.
    because the old site served each language at its own root (e.g. /en/).

    In the new MkDocs structure, pages live at docs/{lang}/{section}.md.
    From a flat section (e.g. en/kadas_gui.md) the correct relative path to
    another flat section is ../map/ (one level up, then into the sibling).
    From a deep section (e.g. en/working_with_vector/expression.md) it would
    be ../../map/, but internal cross-links in deep pages are rare/absent.

    We rewrite:
        /section       →  ../section/
        /section/      →  ../section/
    Only for the known flat section names to avoid touching real external URLs.
    """
    flat_sections = {
        "index", "kadas_gui", "map", "view", "analysis",
        "draw", "gps", "mss", "settings",
    }
    depth = section.count("/")  # 0 for flat, 1 for deep (e.g. appendices/license)
    prefix = "../../" if depth >= 1 else "../"

    def replace_link(m: re.Match) -> str:
        name = m.group(1).rstrip("/")
        if name in flat_sections:
            return f"]({prefix}{name}/)"
        return m.group(0)  # leave unknown root-relative links untouched

    return re.sub(r"\]\(/([\w/]+)/?\)", replace_link, md)


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
    fixed_md = rewrite_html_links(fixed_md)
    fixed_md = rewrite_internal_links(fixed_md, section)

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
    Walk the {lang}/{lang}/ subtree and return a list of (section_key, html_path)
    tuples, sorted so that the top-level index comes first.

    Section keys:
      - "index"                                for {lang}/index.html
      - "analysis"                             for {lang}/analysis/index.html
      - "working_with_vector/expression"       for {lang}/working_with_vector/expression/index.html

    Depth-1 folders that contain a direct index.html are flat sections.
    Depth-1 folders that contain only sub-folders (no direct index.html) are
    group containers — we recurse one level deeper for their pages.
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

    # Iterate depth-1 subdirectories in sorted order
    for sub in sorted(lang_dir.iterdir()):
        if not sub.is_dir():
            continue
        direct_index = sub / "index.html"
        if direct_index.exists():
            # Flat section (e.g. analysis/, draw/)
            sections.append((sub.name, direct_index))
        else:
            # Group container (e.g. working_with_vector/, appendices/)
            # recurse one level deeper
            for subsub in sorted(sub.iterdir()):
                if subsub.is_dir():
                    deep_index = subsub / "index.html"
                    if deep_index.exists():
                        section_key = f"{sub.name}/{subsub.name}"
                        sections.append((section_key, deep_index))

    return sections


# ---------------------------------------------------------------------------
# Media copy
# ---------------------------------------------------------------------------
def _copy_folder(src: Path, dst: Path, label: str) -> None:
    """Copy all files from src into dst, skipping unchanged files."""
    if not src.is_dir():
        print(f"  [WARN] Source not found, skipping: {src}", file=sys.stderr)
        return
    dst.mkdir(parents=True, exist_ok=True)
    copied = 0
    for src_file in src.iterdir():
        if src_file.is_file():
            dst_file = dst / src_file.name
            if not dst_file.exists() or src_file.stat().st_mtime > dst_file.stat().st_mtime:
                shutil.copy2(src_file, dst_file)
                copied += 1
    print(f"  [{label}] {copied} file(s) copied/updated → {dst.relative_to(REPO_ROOT)}")


def copy_media() -> None:
    """Copy media/ (KADAS screenshots) and images/ (QGIS icons) to docs/."""
    _copy_folder(MEDIA_SRC,  MEDIA_DST,  "media")
    _copy_folder(IMAGES_SRC, IMAGES_DST, "images")


# ---------------------------------------------------------------------------
# mkdocs.yml nav skeleton generator
# ---------------------------------------------------------------------------
def generate_nav_skeleton(recovered: dict[str, list[str]]) -> str:
    """
    Build a YAML nav block from the recovered sections.

    Flat sections become top-level nav entries.
    Sections whose key contains a "/" are grouped under their parent heading.

    recovered = { "en": ["index", "kadas_gui", "working_with_vector/expression", ...], ... }
    """
    lines = ["# Paste this nav block into your mkdocs.yml", "nav:"]

    for lang in LANGUAGES:
        if lang not in recovered:
            continue
        lines.append(f"  # --- {lang.upper()} ---")
        emitted_groups: set[str] = set()
        for section in recovered[lang]:
            title = SECTION_TITLES.get(section, section.replace("_", " ").title())
            md_path = f"{lang}/{section}.md"
            if "/" in section:
                group_key = section.split("/")[0]
                group_label = SECTION_GROUPS.get(group_key, group_key.replace("_", " ").title())
                if group_key not in emitted_groups:
                    lines.append(f"  - '{group_label}':")
                    emitted_groups.add(group_key)
                lines.append(f"    - '{title}': '{md_path}'")
            else:
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
    """Recover all sections for a single language. Returns list of section keys."""
    print(f"\n[{lang}] Starting recovery...")
    sections = discover_sections(lang)
    if not sections:
        return []

    lang_out = OUTPUT_ROOT / lang
    if not dry_run:
        lang_out.mkdir(parents=True, exist_ok=True)

    recovered_sections = []
    for section, html_path in sections:
        # Section key may contain a slash for deep pages (e.g. "appendices/shortcuts")
        # — create the intermediate directory if needed.
        out_file = lang_out / f"{section}.md"
        print(f"  [{lang}/{section}] {html_path.relative_to(REPO_ROOT)} → {out_file.relative_to(REPO_ROOT)}")
        if not dry_run:
            out_file.parent.mkdir(parents=True, exist_ok=True)

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