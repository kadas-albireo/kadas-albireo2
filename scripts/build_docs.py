#!/usr/bin/env python3
"""
Build KADAS Albireo documentation for all 4 languages.

Each language is a separate Zensical project (isolated nav + search index).
Shared assets (media/, images/, assets/) live in docs/ and are copied once
into share/docs/html/ after all language builds complete.
The root redirect (docs/index.md → share/docs/html/index.html) is also
copied once at the end.

Usage:
    python3 scripts/build_docs.py [--lang en|de|fr|it]

Without --lang, all 4 languages are built in sequence.
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths — all relative to the repository root
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parent.parent

CONFIGS = {
    "en": REPO_ROOT / "zensical.en.toml",
    "de": REPO_ROOT / "zensical.de.toml",
    "fr": REPO_ROOT / "zensical.fr.toml",
    "it": REPO_ROOT / "zensical.it.toml",
}

# Shared assets that live in docs/ but are referenced by relative paths that
# resolve to share/docs/html/{name}/ (one level above each lang site_dir).
SHARED_ASSET_DIRS = [
    REPO_ROOT / "docs" / "media",
    REPO_ROOT / "docs" / "images",
    REPO_ROOT / "docs" / "assets",
]

# The root index.html (language-selector redirect page).
ROOT_INDEX_SRC = REPO_ROOT / "docs" / "index.md"

OUTPUT_ROOT = REPO_ROOT / "share" / "docs" / "html"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def find_zensical() -> Path:
    """Return the path to the zensical executable inside the venv, or fall
    back to whatever is on PATH."""
    venv_bin = REPO_ROOT / "scripts" / ".venv" / "bin" / "zensical"
    if venv_bin.exists():
        return venv_bin
    venv_bin_win = REPO_ROOT / "scripts" / ".venv" / "Scripts" / "zensical.exe"
    if venv_bin_win.exists():
        return venv_bin_win
    return Path("zensical")


def build_lang(lang: str, zensical: Path) -> None:
    config = CONFIGS[lang]
    print(f"\n{'='*60}")
    print(f"  Building: {lang.upper()}  ({config.name})")
    print(f"{'='*60}")
    result = subprocess.run(
        [str(zensical), "build", "--config-file", str(config)],
        cwd=REPO_ROOT,
    )
    if result.returncode != 0:
        print(f"\nERROR: zensical build failed for '{lang}' (exit {result.returncode})",
              file=sys.stderr)
        sys.exit(result.returncode)


def copy_shared_assets() -> None:
    """Copy media/, images/ and assets/ from docs/ into share/docs/html/.
    These are referenced by relative paths in the built HTML that escape the
    per-language site_dir, resolving to share/docs/html/{name}/.
    """
    print(f"\n{'='*60}")
    print("  Copying shared assets → share/docs/html/")
    print(f"{'='*60}")
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    for src in SHARED_ASSET_DIRS:
        if not src.exists():
            print(f"  SKIP  {src.name}/ (not found)")
            continue
        dst = OUTPUT_ROOT / src.name
        if dst.exists():
            shutil.rmtree(dst)
        shutil.copytree(src, dst)
        print(f"  COPY  docs/{src.name}/ → share/docs/html/{src.name}/")


def copy_root_redirect() -> None:
    """Copy the pre-built root index.html (language redirect) to the output
    root.  The file is a raw HTML fragment, not a Markdown page, so we copy
    it directly rather than building it through Zensical."""
    src = REPO_ROOT / "share" / "docs" / "html_root_index" / "index.html"
    # Fall back: the docs/index.md contains a <meta refresh> HTML fragment.
    # If it hasn't been pre-built, write it directly from the source file.
    root_md = ROOT_INDEX_SRC
    dst = OUTPUT_ROOT / "index.html"
    if src.exists():
        shutil.copy2(src, dst)
        print(f"  COPY  root index.html → share/docs/html/index.html")
    elif root_md.exists():
        # docs/index.md is a bare HTML snippet — copy as-is.
        shutil.copy2(root_md, dst)
        print(f"  COPY  docs/index.md → share/docs/html/index.html")
    else:
        print("  WARN  No root index found; writing a minimal redirect.")
        dst.write_text(
            '<meta http-equiv="refresh" content="0; url=en/">\n',
            encoding="utf-8",
        )


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "--lang",
        choices=list(CONFIGS.keys()),
        help="Build only one language instead of all four.",
    )
    args = parser.parse_args()

    zensical = find_zensical()
    langs = [args.lang] if args.lang else list(CONFIGS.keys())

    for lang in langs:
        build_lang(lang, zensical)

    # Shared assets and root redirect are only written when doing a full build
    # (or when the caller explicitly requests it for a single-lang rebuild).
    copy_shared_assets()
    copy_root_redirect()

    print(f"\n{'='*60}")
    print(f"  Done. Output: share/docs/html/")
    print(f"{'='*60}\n")


if __name__ == "__main__":
    main()