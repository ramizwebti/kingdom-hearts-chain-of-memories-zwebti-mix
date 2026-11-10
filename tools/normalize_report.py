#!/usr/bin/env python3
"""Make the objdiff progress report truthful before it is published.

An empty dimension is reported as 100 percent by the report generator, and
decomp.dev consumes that verbatim, so a project that measures no data at all
publishes "data: 100% matched". Drop the percentages for any dimension whose
total is zero, so the dimension reads as unmeasured rather than finished, then
recompute the category and project rollups from byte and function weights.

Also derive the completion measures decomp.dev renders as linking progress: a
unit is complete when every byte it owns is matched.
"""

import argparse
import json
from pathlib import Path

# Everything the build generates from extracted assets: art, audio, text, FMV.
# They are real cartridge data and the build needs them, but they are not
# decompilation work, so they count as neither matched nor unmatched data.
EXCLUDED_UNITS = ("gen/",)
UNATTRIBUTED_UNITS = ("asm/rodata_",)

DIMENSIONS = {
    "code": ("total_code", ("matched_code_percent", "complete_code_percent", "fuzzy_match_percent")),
    "data": ("total_data", ("matched_data_percent", "complete_data_percent")),
    "functions": ("total_functions", ("matched_functions_percent",)),
}


def amount(measures, key):
    try:
        return int(measures.get(key, 0) or 0)
    except (TypeError, ValueError):
        return 0


def is_complete(measures):
    owned = amount(measures, "total_code") + amount(measures, "total_data")
    if not owned:
        return False
    matched = amount(measures, "matched_code") + amount(measures, "matched_data")
    return matched == owned


def normalize_unit(unit):
    measures = unit.setdefault("measures", {})
    complete = is_complete(measures)
    unit.setdefault("metadata", {})["complete"] = complete

    for total_key, percent_keys in DIMENSIONS.values():
        total = amount(measures, total_key)
        for percent_key in percent_keys:
            if not total:
                measures.pop(percent_key, None)
            elif percent_key.startswith("complete_"):
                measures[percent_key] = 100.0 if complete else 0.0
            else:
                measures.setdefault(percent_key, 0.0)


def aggregate(units):
    result = {"total_units": len(units)}
    complete_units = [u for u in units if u.get("metadata", {}).get("complete")]
    result["complete_units"] = len(complete_units)

    for total_key, percent_keys in DIMENSIONS.values():
        total = sum(amount(u["measures"], total_key) for u in units)
        if not total:
            continue
        result[total_key] = total if total_key == "total_functions" else str(total)
        for percent_key in percent_keys:
            weighted = sum(
                amount(u["measures"], total_key) * float(u["measures"].get(percent_key, 0.0))
                for u in units
            )
            result[percent_key] = weighted / total

    matched_code = sum(amount(u["measures"], "matched_code") for u in units)
    matched_functions = sum(amount(u["measures"], "matched_functions") for u in units)
    if matched_code:
        result["matched_code"] = str(matched_code)
    if matched_functions:
        result["matched_functions"] = matched_functions

    complete_code = sum(amount(u["measures"], "total_code") for u in complete_units)
    if complete_code:
        result["complete_code"] = str(complete_code)
    return result


def declared_category_ids():
    config_path = Path(__file__).resolve().parent.parent / "decomp.yaml"
    ids = set()
    for line in config_path.read_text().splitlines():
        stripped = line.lstrip()
        if stripped.startswith("- id:"):
            ids.add(stripped.split(":", 1)[1].strip())
    return ids


def declared_category_paths():
    """Map each category id to its path prefixes, as listed in decomp.yaml."""
    config_path = Path(__file__).resolve().parent.parent / "decomp.yaml"
    paths, current, in_paths = {}, None, False
    for line in config_path.read_text().splitlines():
        stripped = line.strip()
        if stripped.startswith("- id:"):
            current = stripped.split(":", 1)[1].strip()
            paths[current] = []
            in_paths = False
        elif current is not None and stripped == "paths:":
            in_paths = True
        elif in_paths and stripped.startswith("- "):
            paths[current].append(stripped[2:].strip())
        elif stripped and not stripped.startswith("#"):
            in_paths = False
    return paths


def most_specific(name, cats, paths):
    """Keep only the category whose matching path prefix is longest."""
    if len(cats) < 2:
        return cats
    return [max(cats, key=lambda c: max((len(p) for p in paths.get(c, []) if name.startswith(p)), default=-1))]


def drop_excluded(report):
    report["units"] = [
        u for u in report.get("units", [])
        if not u.get("name", "").startswith(EXCLUDED_UNITS)
    ]
    # ROM data that no translation unit owns yet. It counts toward the project
    # totals but belongs to no subsystem, so leave it uncategorised rather than
    # invent a bucket for it.
    allowed = declared_category_ids()
    paths = declared_category_paths()
    for unit in report["units"]:
        meta = unit.setdefault("metadata", {})
        cats = meta.get("progress_categories") or []
        if unit.get("name", "").startswith(UNATTRIBUTED_UNITS):
            meta["progress_categories"] = []
        else:
            meta["progress_categories"] = most_specific(
                unit.get("name", ""), [c for c in cats if c in allowed], paths)
    used = {c for u in report["units"]
            for c in (u.get("metadata", {}).get("progress_categories") or [])}
    report["categories"] = [
        c for c in report.get("categories", [])
        if c.get("id") in allowed and c.get("id") in used
    ]


def normalize(report):
    drop_excluded(report)
    units = report["units"]
    for unit in units:
        normalize_unit(unit)

    by_category = {}
    for unit in units:
        for category in unit.get("metadata", {}).get("progress_categories", []):
            by_category.setdefault(category, []).append(unit)
    for category in report.get("categories", []):
        category["measures"] = aggregate(by_category.get(category["id"], []))
    report["measures"] = aggregate(units)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    report = json.loads(args.report.read_text())
    normalize(report)
    args.report.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
