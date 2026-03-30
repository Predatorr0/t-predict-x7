#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE_FILES = [
    ROOT / "src/game/client/components/bestclient/menus_bestclient.cpp",
    ROOT / "src/game/client/components/bestclient/menus_bestclient_shop.cpp",
    ROOT / "src/game/client/components/menus_games.cpp",
    ROOT / "src/game/client/components/bestclient/editors/menus_assets_editor.cpp",
    ROOT / "src/game/client/components/menu_media_background.cpp",
]
LANG_FILES = {
    "russian": ROOT / "data/BestClient/languages/russian.txt",
    "german": ROOT / "data/BestClient/languages/german.txt",
}
IDENTICAL_EXCEPTIONS = {
    "2048!",
    "Telegram",
    "Discord",
    "Name:",
}
EFFECTIVE_KEYS = {
    "Editors",
    "Assets editor",
    "Components editor",
    "HUD editor",
    "Create mixed assets or jump to the name plate editor.",
    "Open a dedicated component toggles page.",
    "Edit HUD positions directly above the live game.",
    "Looks like you're on a server where this feature is forbidden",
}


def parse_bclocalize_keys(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    pattern = re.compile(r'BCLocalize\("((?:\\.|[^"\\])*)"')
    return {bytes(m.group(1), "utf-8").decode("unicode_escape") for m in pattern.finditer(text)}


def parse_language_file_runtime(path: Path) -> tuple[dict[tuple[str, str], str], list[str], list[str]]:
    mapping: dict[tuple[str, str], str] = {}
    malformed_contexts: list[str] = []
    malformed_replacements: list[str] = []
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    i = 0
    line_no = 0
    while i < len(lines):
        line_no += 1
        key = lines[i]
        if not key or key.startswith("#"):
            i += 1
            continue

        context = ""
        if key.startswith("["):
            if not key.endswith("]"):
                malformed_contexts.append(f"line {line_no}: {key}")
                i += 1
                continue
            context = key[1:-1]
            i += 1
            if i >= len(lines):
                malformed_replacements.append(f"line {line_no}: unexpected EOF after context {key}")
                break
            line_no += 1
            key = lines[i]

        i += 1
        if i >= len(lines):
            malformed_replacements.append(f"line {line_no}: unexpected EOF after original {key}")
            break
        replacement = lines[i]
        line_no += 1
        if not replacement.startswith("== "):
            malformed_replacements.append(f"line {line_no}: original={key} replacement={replacement}")
            i += 1
            continue

        value = replacement[3:]
        if value:
            mapping.setdefault((key, context), value)
        i += 1

    return mapping, malformed_contexts, malformed_replacements


def main() -> int:
    keys: set[str] = set()
    for source in SOURCE_FILES:
        keys.update(parse_bclocalize_keys(source))
    keys = set(sorted(keys))

    failed = False
    for lang_name, lang_file in LANG_FILES.items():
        mapping, malformed_contexts, malformed_replacements = parse_language_file_runtime(lang_file)
        effective = {key: mapping[(key, "")] for key in keys if (key, "") in mapping}
        missing = sorted(key for key in keys if key not in effective)
        identical = sorted(key for key in keys if key in effective and effective[key].strip() == key.strip() and key not in IDENTICAL_EXCEPTIONS)

        print(
            f"[{lang_name}] total_keys={len(keys)} missing={len(missing)} identical={len(identical)} "
            f"malformed_context={len(malformed_contexts)} malformed_replacement={len(malformed_replacements)}"
        )
        for key in missing:
            print(f"  MISSING: {key}")
        for key in identical:
            print(f"  IDENTICAL: {key}")
        for entry in malformed_contexts:
            print(f"  MALFORMED_CONTEXT: {entry}")
        for entry in malformed_replacements:
            print(f"  MALFORMED_REPLACEMENT: {entry}")
        for key in sorted(EFFECTIVE_KEYS):
            print(f"  EFFECTIVE: {key} => {effective.get(key, '<missing>')}")
        if missing or identical or malformed_contexts or malformed_replacements:
            failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
