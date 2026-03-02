#!/usr/bin/env python3

from __future__ import annotations

import csv
import pathlib
import re
from dataclasses import dataclass


ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "m-kconfig" / "header-fixes"
KCONFIG_INCLUDE = ROOT / "m-kconfig" / "include" / "kconfig"

SCAN_ROOTS = [
    ROOT / "m-karma" / "src",
    ROOT / "m-bz3" / "src",
]

# Exclude the implementation directories that were copied into m-kconfig.
EXCLUDED_SCAN_DIRS = [
    ROOT / "m-karma" / "src" / "common" / "config",
    ROOT / "m-karma" / "src" / "common" / "data",
    ROOT / "m-karma" / "src" / "common" / "i18n",
    ROOT / "m-karma" / "src" / "common" / "serialization",
]

SCAN_EXTS = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".ipp",
    ".inl",
    ".tpp",
}

# New public header -> legacy header(s) used by m-karma/m-bz3 before m-kconfig existed.
HEADER_MAP: dict[str, list[str]] = {
    "m-kconfig/include/kconfig/helpers.hpp": [
        "m-karma/include/karma/common/config/helpers.hpp",
        "m-karma/src/common/config/helpers.hpp",
    ],
    "m-kconfig/include/kconfig/store.hpp": [
        "m-karma/include/karma/common/config/store.hpp",
        "m-karma/src/common/config/store.hpp",
    ],
    "m-kconfig/include/kconfig/validation.hpp": [
        "m-karma/include/karma/common/config/validation.hpp",
        "m-karma/src/common/config/validation.hpp",
    ],
    "m-kconfig/include/kconfig/i18n.hpp": [
        "m-karma/include/karma/common/i18n/i18n.hpp",
        "m-karma/src/common/i18n/i18n.hpp",
    ],
    "m-kconfig/include/kconfig/json.hpp": [
        "m-karma/include/karma/common/serialization/json.hpp",
        "m-karma/src/common/serialization/json.hpp",
    ],
    "m-kconfig/include/kconfig/data/path_resolver.hpp": [
        "m-karma/include/karma/common/data/path_resolver.hpp",
        "m-karma/src/common/data/path_resolver.hpp",
    ],
    "m-kconfig/include/kconfig/data/path_utils.hpp": [
        "m-karma/src/common/data/path_utils.hpp",
    ],
    "m-kconfig/include/kconfig/data/directory_override.hpp": [
        "m-karma/src/common/data/directory_override.hpp",
        "m-karma/src/karma/common/data/directory_override.hpp",
    ],
    "m-kconfig/include/kconfig/data/root_policy.hpp": [
        "m-karma/src/common/data/root_policy.hpp",
    ],
    # No pre-existing equivalent found under m-karma/include or m-karma/src.
    "m-kconfig/include/kconfig/trace.hpp": [],
}

NAMESPACE_MAP: list[tuple[str, str]] = [
    ("kconfig::common::config", "karma::common::config"),
    ("kconfig::common::data::path_utils", "karma::common::data::path_utils"),
    ("kconfig::common::data", "karma::common::data"),
    ("kconfig::common::i18n", "karma::common::i18n"),
    ("kconfig::common::serialization", "karma::common::serialization"),
]

HEADER_NAMESPACE_MAP: dict[str, str] = {
    "m-kconfig/include/kconfig/helpers.hpp": "karma::common::config",
    "m-kconfig/include/kconfig/store.hpp": "karma::common::config",
    "m-kconfig/include/kconfig/validation.hpp": "karma::common::config",
    "m-kconfig/include/kconfig/i18n.hpp": "karma::common::i18n",
    "m-kconfig/include/kconfig/json.hpp": "karma::common::serialization",
    "m-kconfig/include/kconfig/data/path_resolver.hpp": "karma::common::data",
    "m-kconfig/include/kconfig/data/path_utils.hpp": "karma::common::data::path_utils",
    "m-kconfig/include/kconfig/data/directory_override.hpp": "karma::common::data",
    "m-kconfig/include/kconfig/data/root_policy.hpp": "karma::common::data",
    "m-kconfig/include/kconfig/trace.hpp": "",
}

TYPE_MAP: list[tuple[str, str, str, str]] = [
    ("m-kconfig/include/kconfig/store.hpp", "ConfigFileSpec", "karma::common::config::ConfigFileSpec", r"\b(?:karma::)?common::config::ConfigFileSpec\b"),
    ("m-kconfig/include/kconfig/store.hpp", "ConfigStore", "karma::common::config::ConfigStore", r"\b(?:karma::)?common::config::ConfigStore\b"),
    ("m-kconfig/include/kconfig/validation.hpp", "RequiredType", "karma::common::config::RequiredType", r"\b(?:karma::)?common::config::RequiredType\b"),
    ("m-kconfig/include/kconfig/validation.hpp", "RequiredKey", "karma::common::config::RequiredKey", r"\b(?:karma::)?common::config::RequiredKey\b"),
    ("m-kconfig/include/kconfig/validation.hpp", "ValidationIssue", "karma::common::config::ValidationIssue", r"\b(?:karma::)?common::config::ValidationIssue\b"),
    ("m-kconfig/include/kconfig/i18n.hpp", "I18n", "karma::common::i18n::I18n", r"\b(?:karma::)?common::i18n::I18n\b"),
    ("m-kconfig/include/kconfig/i18n.hpp", "RuntimeRole", "karma::common::i18n::I18n::RuntimeRole", r"\b(?:karma::)?common::i18n::I18n::RuntimeRole\b"),
    ("m-kconfig/include/kconfig/json.hpp", "Value", "karma::common::serialization::Value", r"\b(?:karma::)?common::serialization::Value\b"),
    ("m-kconfig/include/kconfig/data/path_resolver.hpp", "ConfigLayerSpec", "karma::common::data::ConfigLayerSpec", r"\b(?:karma::)?common::data::ConfigLayerSpec\b"),
    ("m-kconfig/include/kconfig/data/path_resolver.hpp", "ConfigLayer", "karma::common::data::ConfigLayer", r"\b(?:karma::)?common::data::ConfigLayer\b"),
    ("m-kconfig/include/kconfig/data/path_resolver.hpp", "DataPathSpec", "karma::common::data::DataPathSpec", r"\b(?:karma::)?common::data::DataPathSpec\b"),
    ("m-kconfig/include/kconfig/data/directory_override.hpp", "DataDirectoryOverrideResult", "karma::common::data::DataDirectoryOverrideResult", r"\b(?:karma::)?common::data::DataDirectoryOverrideResult\b"),
    ("m-kconfig/include/kconfig/data/root_policy.hpp", "RootPathPolicy", "karma::common::data::RootPathPolicy", r"\b(?:karma::)?common::data::RootPathPolicy\b"),
]


@dataclass(frozen=True)
class FunctionSpec:
    new_header: str
    new_symbol: str
    legacy_symbol: str
    legacy_namespace: str
    legacy_headers: tuple[str, ...]
    pattern_strings: tuple[str, ...]


@dataclass(frozen=True)
class Ref:
    file: str
    line: int
    text: str
    pattern: str


def write_list(path: pathlib.Path, rows: list[str]) -> None:
    path.write_text("\n".join(rows) + ("\n" if rows else ""), encoding="utf-8")


def write_tsv(path: pathlib.Path, header: list[str], rows: list[list[str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f, delimiter="\t")
        writer.writerow(header)
        writer.writerows(rows)


def relative(path: pathlib.Path) -> str:
    return path.relative_to(ROOT).as_posix()


def header_list() -> list[str]:
    return sorted(relative(p) for p in KCONFIG_INCLUDE.rglob("*.hpp"))


def load_curated_functions() -> list[tuple[str, str]]:
    curated_path = OUT_DIR / "03_functions_curated.tsv"
    rows: list[tuple[str, str]] = []
    with curated_path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        for row in reader:
            header = row["header"].strip()
            function = row["function"].strip()
            if header and function:
                rows.append((header, function))
    return rows


def split_symbol(symbol: str) -> tuple[str | None, str]:
    if "::" not in symbol:
        return None, symbol.split("(", 1)[0]
    prefix, rest = symbol.split("::", 1)
    return prefix, rest.split("(", 1)[0]


def map_legacy_name(new_header: str, new_symbol: str, base_name: str) -> str:
    if new_header == "m-kconfig/include/kconfig/validation.hpp" and base_name == "ClientRequiredKeys":
        return "ServerRequiredKeys"
    return base_name


def is_unique_config_function(name: str) -> bool:
    return name.startswith("ReadRequired") or name in {"ValidateRequiredKeys", "ServerRequiredKeys"}


def function_patterns(new_header: str, class_name: str | None, legacy_base: str) -> list[str]:
    if new_header == "m-kconfig/include/kconfig/trace.hpp":
        return []

    if class_name == "ConfigStore":
        return [rf"\bConfigStore::\s*{re.escape(legacy_base)}\s*\("]

    if class_name == "I18n":
        return [
            rf"\b(?:karma::)?common::i18n::Get\(\)\s*\.\s*{re.escape(legacy_base)}\s*\(",
            rf"\bi18n\s*\.\s*{re.escape(legacy_base)}\s*\(",
        ]

    if new_header == "m-kconfig/include/kconfig/i18n.hpp" and legacy_base == "Get":
        return [r"\b(?:karma::)?common::i18n::Get\s*\("]

    if new_header == "m-kconfig/include/kconfig/json.hpp":
        return [rf"\b(?:karma::)?common::serialization::{re.escape(legacy_base)}\s*\("]

    if new_header in {
        "m-kconfig/include/kconfig/helpers.hpp",
        "m-kconfig/include/kconfig/validation.hpp",
    }:
        patterns = [rf"\b(?:karma::)?common::config::{re.escape(legacy_base)}\s*\("]
        if is_unique_config_function(legacy_base):
            patterns.append(rf"\b{re.escape(legacy_base)}\s*\(")
        return patterns

    if new_header == "m-kconfig/include/kconfig/data/path_resolver.hpp":
        patterns = [rf"\b(?:karma::)?common::data::{re.escape(legacy_base)}\s*\("]
        if legacy_base in {
            "SetDataRootOverride",
            "SetUserConfigRootOverride",
            "LoadJsonFile",
            "UserConfigDirectory",
            "LoadConfigLayers",
            "MergeJsonObjects",
            "CollectAssetEntries",
            "SetDataPathSpec",
            "ResolveConfiguredAsset",
            "DataRoot",
        }:
            patterns.append(rf"\b{re.escape(legacy_base)}\s*\(")
        return patterns

    if new_header == "m-kconfig/include/kconfig/data/path_utils.hpp":
        return [
            rf"\b(?:karma::)?common::data::path_utils::{re.escape(legacy_base)}\s*\(",
            rf"\b{re.escape(legacy_base)}\s*\(",
        ]

    if new_header in {
        "m-kconfig/include/kconfig/data/directory_override.hpp",
        "m-kconfig/include/kconfig/data/root_policy.hpp",
    }:
        return [
            rf"\b(?:karma::)?common::data::{re.escape(legacy_base)}\s*\(",
            rf"\b{re.escape(legacy_base)}\s*\(",
        ]

    return [rf"\b{re.escape(legacy_base)}\s*\("]


def build_function_specs(curated: list[tuple[str, str]]) -> list[FunctionSpec]:
    specs: list[FunctionSpec] = []
    for new_header, new_symbol in curated:
        class_name, base_name = split_symbol(new_symbol)
        legacy_base = map_legacy_name(new_header, new_symbol, base_name)
        legacy_symbol = f"{class_name}::{legacy_base}" if class_name else legacy_base
        legacy_namespace = HEADER_NAMESPACE_MAP.get(new_header, "")
        patterns = tuple(function_patterns(new_header, class_name, legacy_base))
        specs.append(
            FunctionSpec(
                new_header=new_header,
                new_symbol=new_symbol,
                legacy_symbol=legacy_symbol,
                legacy_namespace=legacy_namespace,
                legacy_headers=tuple(HEADER_MAP.get(new_header, [])),
                pattern_strings=patterns,
            )
        )
    return specs


def is_excluded(path: pathlib.Path) -> bool:
    for excluded in EXCLUDED_SCAN_DIRS:
        try:
            path.relative_to(excluded)
            return True
        except ValueError:
            continue
    return False


def scan_files() -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    for root in SCAN_ROOTS:
        for path in root.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix not in SCAN_EXTS:
                continue
            if is_excluded(path):
                continue
            files.append(path)
    return sorted(files)


def include_tokens_for_legacy_header(legacy_header: str) -> list[str]:
    tokens: list[str] = []
    if legacy_header.startswith("m-karma/include/"):
        tokens.append(legacy_header.removeprefix("m-karma/include/"))
    if legacy_header.startswith("m-karma/src/"):
        tokens.append(legacy_header.removeprefix("m-karma/src/"))
    return tokens


def gather_lines(scan_paths: list[pathlib.Path]) -> dict[str, list[str]]:
    content: dict[str, list[str]] = {}
    for path in scan_paths:
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            continue
        content[relative(path)] = lines
    return content


def strip_inline_comment(line: str) -> str:
    return line.split("//", 1)[0]


def find_include_refs(lines_by_file: dict[str, list[str]]) -> dict[str, list[Ref]]:
    out: dict[str, list[Ref]] = {h: [] for h in HEADER_MAP}
    include_re = re.compile(r"^\s*#\s*include\s*[<\"]([^\">]+)[\">]")

    header_tokens: dict[str, set[str]] = {}
    for new_header, legacy_headers in HEADER_MAP.items():
        tokens: set[str] = set()
        for legacy_header in legacy_headers:
            tokens.update(include_tokens_for_legacy_header(legacy_header))
        header_tokens[new_header] = tokens

    for file_rel, lines in lines_by_file.items():
        for lineno, line in enumerate(lines, start=1):
            m = include_re.match(line)
            if not m:
                continue
            include_target = m.group(1)
            for new_header, tokens in header_tokens.items():
                if include_target in tokens:
                    out[new_header].append(Ref(file=file_rel, line=lineno, text=line.strip(), pattern=include_target))
    return out


def find_function_refs(specs: list[FunctionSpec], lines_by_file: dict[str, list[str]]) -> dict[tuple[str, str], list[Ref]]:
    compiled: dict[tuple[str, str], list[re.Pattern[str]]] = {}
    for spec in specs:
        patterns = [re.compile(p) for p in spec.pattern_strings]
        compiled[(spec.new_header, spec.new_symbol)] = patterns

    out: dict[tuple[str, str], list[Ref]] = {(s.new_header, s.new_symbol): [] for s in specs}

    for file_rel, lines in lines_by_file.items():
        for lineno, raw_line in enumerate(lines, start=1):
            line = strip_inline_comment(raw_line)
            if not line.strip():
                continue
            for spec in specs:
                key = (spec.new_header, spec.new_symbol)
                for pat in compiled[key]:
                    if pat.search(line):
                        out[key].append(Ref(file=file_rel, line=lineno, text=raw_line.strip(), pattern=pat.pattern))
                        break
    return out


def find_type_refs(lines_by_file: dict[str, list[str]]) -> dict[tuple[str, str], list[Ref]]:
    specs: list[tuple[str, str, re.Pattern[str]]] = []
    for new_header, new_type, _legacy_type, pattern in TYPE_MAP:
        specs.append((new_header, new_type, re.compile(pattern)))

    out: dict[tuple[str, str], list[Ref]] = {(h, t): [] for (h, t, _, _) in TYPE_MAP}

    for file_rel, lines in lines_by_file.items():
        for lineno, raw_line in enumerate(lines, start=1):
            line = strip_inline_comment(raw_line)
            if not line.strip():
                continue
            for new_header, new_type, pattern in specs:
                if pattern.search(line):
                    out[(new_header, new_type)].append(
                        Ref(file=file_rel, line=lineno, text=raw_line.strip(), pattern=pattern.pattern)
                    )

    return out


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    headers = header_list()
    write_list(OUT_DIR / "00_headers.txt", headers)

    write_tsv(
        OUT_DIR / "20_header_map.tsv",
        ["new_header", "legacy_header", "legacy_exists"],
        [
            [new_h, legacy_h, "False" if not legacy_h else str((ROOT / legacy_h).exists())]
            for new_h in sorted(HEADER_MAP)
            for legacy_h in (HEADER_MAP[new_h] if HEADER_MAP[new_h] else [""])
        ],
    )

    write_tsv(
        OUT_DIR / "21_namespace_map.tsv",
        ["new_namespace", "legacy_namespace"],
        [[a, b] for (a, b) in NAMESPACE_MAP],
    )

    curated = load_curated_functions()
    specs = build_function_specs(curated)

    write_tsv(
        OUT_DIR / "22_function_map.tsv",
        [
            "new_header",
            "new_symbol",
            "legacy_symbol",
            "legacy_namespace",
            "legacy_headers",
            "search_patterns",
        ],
        [
            [
                s.new_header,
                s.new_symbol,
                s.legacy_symbol,
                s.legacy_namespace,
                ";".join(s.legacy_headers),
                ";".join(s.pattern_strings),
            ]
            for s in specs
        ],
    )

    write_tsv(
        OUT_DIR / "23_type_map.tsv",
        ["new_header", "new_type", "legacy_type", "search_pattern"],
        [[h, t, lt, p] for (h, t, lt, p) in TYPE_MAP],
    )

    scan_paths = scan_files()
    write_list(OUT_DIR / "30_scan_files.txt", [relative(p) for p in scan_paths])

    lines_by_file = gather_lines(scan_paths)

    include_refs = find_include_refs(lines_by_file)
    function_refs = find_function_refs(specs, lines_by_file)
    type_refs = find_type_refs(lines_by_file)

    include_rows: list[list[str]] = []
    for new_header in sorted(include_refs):
        for ref in include_refs[new_header]:
            include_rows.append([new_header, ref.file, str(ref.line), ref.pattern, ref.text])
    write_tsv(
        OUT_DIR / "31_include_refs.tsv",
        ["new_header", "file", "line", "matched_include", "text"],
        include_rows,
    )

    function_rows: list[list[str]] = []
    spec_by_key = {(s.new_header, s.new_symbol): s for s in specs}
    for key in sorted(function_refs):
        spec = spec_by_key[key]
        for ref in function_refs[key]:
            function_rows.append(
                [
                    spec.new_header,
                    spec.new_symbol,
                    spec.legacy_symbol,
                    ref.file,
                    str(ref.line),
                    ref.pattern,
                    ref.text,
                ]
            )
    write_tsv(
        OUT_DIR / "32_function_refs.tsv",
        ["new_header", "new_symbol", "legacy_symbol", "file", "line", "matched_pattern", "text"],
        function_rows,
    )

    type_rows: list[list[str]] = []
    type_legacy = {(h, t): lt for (h, t, lt, _p) in TYPE_MAP}
    for key in sorted(type_refs):
        for ref in type_refs[key]:
            type_rows.append([key[0], key[1], type_legacy[key], ref.file, str(ref.line), ref.pattern, ref.text])
    write_tsv(
        OUT_DIR / "33_type_refs.tsv",
        ["new_header", "new_type", "legacy_type", "file", "line", "matched_pattern", "text"],
        type_rows,
    )

    used_functions: set[tuple[str, str]] = {k for k, refs in function_refs.items() if refs}

    used_headers: set[str] = set()
    for h, refs in include_refs.items():
        if refs:
            used_headers.add(h)
    for (h, _sym), refs in function_refs.items():
        if refs:
            used_headers.add(h)
    for (h, _type), refs in type_refs.items():
        if refs:
            used_headers.add(h)

    all_headers = sorted(set(headers))
    unused_headers = [h for h in all_headers if h not in used_headers]

    write_list(OUT_DIR / "final_used_files.txt", sorted(used_headers))
    write_list(OUT_DIR / "final_unused_files.txt", unused_headers)

    final_used_function_rows: list[list[str]] = []
    final_unused_function_rows: list[list[str]] = []
    for spec in specs:
        key = (spec.new_header, spec.new_symbol)
        count = len(function_refs[key])
        row = [
            spec.new_header,
            spec.new_symbol,
            spec.legacy_symbol,
            spec.legacy_namespace,
            str(count),
            "mapped" if spec.pattern_strings else "no_legacy_mapping",
        ]
        if key in used_functions:
            final_used_function_rows.append(row)
        else:
            final_unused_function_rows.append(row)

    write_tsv(
        OUT_DIR / "final_used_functions.tsv",
        ["header", "function", "legacy_symbol", "legacy_namespace", "refs", "status"],
        final_used_function_rows,
    )
    write_tsv(
        OUT_DIR / "final_unused_functions.tsv",
        ["header", "function", "legacy_symbol", "legacy_namespace", "refs", "status"],
        final_unused_function_rows,
    )

    summary = [
        f"headers_total={len(all_headers)}",
        f"functions_total={len(specs)}",
        f"scan_files_total={len(scan_paths)}",
        f"include_refs_total={sum(len(v) for v in include_refs.values())}",
        f"function_refs_total={sum(len(v) for v in function_refs.values())}",
        f"type_refs_total={sum(len(v) for v in type_refs.values())}",
        f"used_files_total={len(used_headers)}",
        f"unused_files_total={len(unused_headers)}",
        f"used_functions_total={len(final_used_function_rows)}",
        f"unused_functions_total={len(final_unused_function_rows)}",
        "",
        "scan_scope=m-karma/src + m-bz3/src",
        "excluded=m-karma/src/common/{config,data,i18n,serialization}",
        "mapping=legacy header+namespace+symbol mapping from m-kconfig/include to m-karma paths",
    ]
    write_list(OUT_DIR / "final_summary.txt", summary)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
