#!/usr/bin/env python3
################################################################################
# Copyright (C) 2026  Azhar Tanweer
# Contact: azhar.tanweer404@gmail.com
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
"""
Generate param_meta_arducopter.cpp from ArduPilot's published Copter parameter
lists — one page per stable release, 4.0.0 onwards, as offered by the version
picker on https://ardupilot.org/copter/docs/parameters.html.

Every release's page is parsed and the results merged: a parameter whose
documentation did not change between releases is stored once, tagged with
the set of releases it applies to, and one that did change gets one record
per distinct version of its text. The GCS picks the record matching the
firmware the vehicle reports.

Not run at build time: its source is the web, and a build that needs the
network — or changes under you because someone edited the wiki — is worse
than a checked-in table refreshed on purpose. Rerun it when a new stable
release appears and commit the result.

Usage: gen_param_meta_arducopter.py <out_cpp> [<cache_dir>]

<cache_dir> keeps the downloaded pages, and pages already in it are not
fetched again.
"""

import json
import os
import re
import sys

from param_meta_common import (cached, clean_bits, clean_values, emit, is_int,
                               merge, new_record, norm, parse_html, table_rows,
                               values_are_enum)

BASE_URL     = "https://ardupilot.org/copter/"
VERSIONS_URL = BASE_URL + "_static/parameters-Copter.json"
PAGE_URL     = BASE_URL + "docs/"


# ── Page → parameter records ──────────────────────────────────────────────────

def heading_of(section):
    for tag in ("h1", "h2", "h3"):
        h = section.elems(tag)
        if h:
            return tag, norm(h[0].text()).rstrip("¶").strip()
    return None, None


# Values and bits are a nested two-column table on most pages, but a few
# older ones print them flat: "0: Servo 1, 1: Servo 2, ...".
_FLAT_PAIR = re.compile(r"(-?\d+(?:\.\d+)?)\s*:\s*(.*?)(?=,\s*-?\d+(?:\.\d+)?\s*:|$)")


def value_pairs(cell):
    inner = cell.elems("table")
    if inner:
        _, sub = table_rows(inner[0])
        return tuple((norm(a.text()), norm(b.text())) for a, b, *_ in sub)
    return tuple((a, b.strip()) for a, b in _FLAT_PAIR.findall(norm(cell.text())))


def parse_range(v):
    # "0 to 60000" on newer pages, "0 - 60000" on older ones.
    for sep in (" to ", " - "):
        lo, found, hi = v.partition(sep)
        if found:
            try:
                return (float(lo), float(hi))
            except ValueError:
                return None
    return None


def parse_param(section, group, warn):
    _, title = heading_of(section)
    name, _, display = title.partition(":")
    # "ATC_RAT_RLL_P (AC_AttitudeControl_Multi)": one parameter documented
    # once per library that can own it, with different ranges and defaults.
    name, _, variant = name.strip().partition(" (")
    p = new_record(name.strip(), group)
    p["variant"] = variant.rstrip(")") or None
    p["display"] = display.strip()
    desc = []
    for el in section.elems():
        if el.tag == "div" and "line-block" in el.attrs.get("class", ""):
            note = norm(el.text()).lower()
            if "reboot required" in note: p["reboot"] = True
            if "advanced users" in note:  p["advanced"] = True
        elif el.tag == "p":
            t = norm(el.text())
            if t: desc.append(t)
        elif el.tag == "table":
            head, rows = table_rows(el)
            if not rows:
                continue
            for h, cell in zip(head, rows[0]):
                if h in ("Values", "Bitmask"):
                    pairs = value_pairs(cell)
                    if pairs:
                        p["values" if h == "Values" else "bitmask"] = pairs
                    continue
                v = norm(cell.text())
                if h == "Range":            p["range"] = parse_range(v)
                elif h == "Increment":
                    try:               p["increment"] = float(v)
                    except ValueError: pass
                elif h == "Units":          p["units"] = v
                elif h == "ReadOnly":       p["readonly"] = v.lower() == "true"
                elif h == "Calibration":    p["calibration"] = v != "0"
                elif h == "Volatile":       p["volatile"] = v.lower() == "true"
                elif h == "RebootRequired": p["reboot"] = p["reboot"] or v.lower() == "true"
                else:
                    warn(f"{p['name']}: unknown column '{h}' ignored")
    p["desc"]    = "\n\n".join(desc)
    p["bitmask"] = clean_bits(p["bitmask"])
    p["values"]  = clean_values(p["values"])
    p["kind"]    = classify(p)
    return p


def walk(node, group, out, warn):
    for el in node.elems():
        if el.tag == "section":
            tag, title = heading_of(el)
            if tag == "h3":
                out.append(parse_param(el, group, warn))
            else:
                walk(el, title if tag == "h2" else group, out, warn)
        else:
            walk(el, group, out, warn)


def parse_page(html, warn):
    params = []
    walk(parse_html(html), None, params, warn)
    return params


# What the page says each parameter is. It never names a storage type — that
# only the vehicle knows, in PARAM_VALUE's param_type — so a plain number is
# called an integer when everything the page does say about it is whole:
# its step, both ends of its range. The GCS prefers the vehicle's word over
# this guess wherever it has one. Values make an Enum only when they are the
# whole story (values_are_enum).
def classify(p):
    if p["bitmask"]:
        return "Bitmask"
    if values_are_enum(p["values"], p["range"]):
        return "Enum"
    inc, rng = p["increment"], p["range"]
    if inc is not None and is_int(inc) and (rng is None or (is_int(rng[0]) and is_int(rng[1]))):
        return "Int"
    return "Float"


# ── Fetching ──────────────────────────────────────────────────────────────────

def stable_versions(cache_dir):
    """[(major, minor, patch, page)] for every stable release, oldest first."""
    picker = json.loads(cached(cache_dir, "parameters-Copter.json", VERSIONS_URL))
    out = []
    for label, page in picker.items():
        m = re.fullmatch(r"Copter stable V(\d+)\.(\d+)\.(\d+)", label)
        if m:
            out.append((int(m[1]), int(m[2]), int(m[3]), page))
    out.sort()
    return out


# ── Emit ──────────────────────────────────────────────────────────────────────

# Of a parameter's variants, the one a copter is most likely to be running:
# sorted first, so a lookup that knows nothing more lands on it.
PREFERRED_VARIANTS = {
    "AC_AttitudeControl_Multi", "AC_AutoTune_Multi",
    "AP_BattMonitor_Analog", "AP_MotorsHeli_Single",
}


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    out_path  = sys.argv[1]
    cache_dir = sys.argv[2] if len(sys.argv) == 3 else None
    if cache_dir:
        os.makedirs(cache_dir, exist_ok=True)

    versions = stable_versions(cache_dir)
    warnings = set()

    pages = []
    for ma, mi, pa, page in versions:
        params = parse_page(cached(cache_dir, page, PAGE_URL + page), warnings.add)
        pages.append((f"V{ma}.{mi}.{pa}", params))
        print(f"V{ma}.{mi}.{pa}: {len(params)} parameters", file=sys.stderr)

    records = merge(pages, warnings.add)
    for w in sorted(warnings):
        print(f"warning: {w}", file=sys.stderr)

    first, last = versions[0], versions[-1]
    emit(records, [v[:3] for v in versions], out_path,
         script="gen_param_meta_arducopter.py",
         source=PAGE_URL + "parameters-Copter-stable-V*.html",
         summary=f"{len(versions)} stable releases, V{first[0]}.{first[1]}.{first[2]} "
                 f"to V{last[0]}.{last[1]}.{last[2]}",
         symbol="param_docs_arducopter", display_name="Copter",
         minor_releases=False, preferred_variants=PREFERRED_VARIANTS)
    print(f"{len(records)} records, {len({p['name'] for p, _ in records})} names "
          f"-> {out_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
