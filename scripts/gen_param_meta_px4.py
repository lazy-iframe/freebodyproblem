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
Generate param_meta_px4.cpp from PX4's published parameter reference — one
page per release, as offered by the version dropdown on
https://docs.px4.io/main/en/advanced_config/parameter_reference.

The releases are the dropdown's numbered ones (v1.12 on); "main" is the
development branch and is left out. PX4 publishes one page per minor release,
so a 1.15.4 vehicle reads v1.15's.

Two page layouts are read. Up to v1.15 each section is one table, a row per
parameter: name and type, description with values or bits folded in,
"min > max (incr)" or "[min, max] (incr)", default, units. From v1.16 each
parameter is its own heading, paragraphs and a small table of reboot, min,
max, increment, default and unit.

Records are merged across releases as for ArduPilot (param_meta_common.py)
and the result checked in; rerun when a new release appears.

Usage: gen_param_meta_px4.py <out_cpp> [<cache_dir>]
"""

import json
import os
import re
import sys

from param_meta_common import (FLT_MAX, cached, clean_bits, clean_values, emit,
                               merge, new_record, norm, parse_html, table_rows,
                               to_float, values_are_enum)

SITE      = "https://docs.px4.io/"
PAGE_PATH = "/en/advanced_config/parameter_reference"
MAIN_URL  = SITE + "main" + PAGE_PATH

# PX4 stores every parameter as one of these two.
_TYPES = {"INT32": "Int", "FLOAT": "Float"}

# "Enabled (1)" / "Disabled (0)": how both layouts print a boolean's default,
# and the only mark a boolean carries.
_BOOL_DEFAULT = re.compile(r"(Enabled|Disabled) \((\d)\)")
_BOOL_VALUES  = ((0.0, "Disabled"), (1.0, "Enabled"))

_PAIR = re.compile(r"^\s*(-?\d+(?:\.\d+)?)\s*:\s*(.*)$", re.S)
_NUM  = r"(-?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?|\?)"
# "0 > 1 (0.01)" up to v1.13, "[0, 1] (0.01)" after; an open end is "?".
_RANGE_OLD = re.compile(rf"^\[?\s*{_NUM}?\s*(?:>|,)\s*{_NUM}?\s*\]?\s*(?:\(\s*{_NUM}\s*\))?$")
_INCR_ONLY = re.compile(rf"^\(\s*{_NUM}\s*\)$")


def list_pairs(ul):
    """<li>0: label</li> (v1.16 on: <code>0</code>: label) → (("0", "label"), ...)."""
    out = []
    for li in ul.elems("li"):
        m = _PAIR.match(norm(li.text()))
        if m:
            out.append((m[1], m[2].strip()))
    return tuple(out)


def set_default(p, text, warn):
    text = norm(text)
    if not text:
        return
    m = _BOOL_DEFAULT.fullmatch(text)
    if m:
        p["default"] = float(m[2])
        p["boolean"] = True
        return
    v = to_float(text)
    if v is None:
        warn(f"{p['name']}: default '{text}' not a number, ignored")
    p["default"] = v


def set_range(p, lo, hi):
    lo, hi = to_float(lo), to_float(hi)
    if lo is not None or hi is not None:
        p["range"] = (-FLT_MAX if lo is None else lo, FLT_MAX if hi is None else hi)


def finish(p):
    p["display"] = p["display"].rstrip(".").strip()
    p["desc"]    = "\n\n".join(p["desc"])
    p["bitmask"] = clean_bits(p["bitmask"])
    p["values"]  = clean_values(p["values"])
    if p.pop("boolean", False) and not p["values"] and not p["bitmask"]:
        p["values"] = _BOOL_VALUES
    p["kind"] = classify(p)
    p.pop("type")
    return p


def name_and_type(text):
    """'ATT_W_ACC (FLOAT)' → ('ATT_W_ACC', 'FLOAT')."""
    m = re.match(r"^\s*([A-Za-z0-9_]+)\s*\(\s*([A-Z0-9]+)\s*\)", text)
    return (m[1], m[2]) if m else (None, None)


# ── v1.12 – v1.15: a table per section, a row per parameter ───────────────────

def parse_row(cells, group, warn):
    name, typ = name_and_type(norm(cells[0].text()))
    if not name:
        return None
    p = new_record(name, group)
    p["type"], p["desc"] = typ, []

    # The description cell runs: short text, then <p> paragraphs ("Comment:",
    # "Reboot required:"), then "Values:" / "Bitmask:" and a list.
    short, pending = [], None
    for c in cells[1].children:
        if isinstance(c, str):
            if not p["desc"] and pending is None:
                short.append(c)
            continue
        t = norm(c.text())
        if c.tag == "p":
            if t.startswith("Comment:"):
                p["desc"].append(t[len("Comment:"):].strip())
            elif t.startswith("Reboot required:"):
                p["reboot"] = "true" in t.lower()
            elif t:
                p["desc"].append(t)
        elif c.tag in ("strong", "b") and t in ("Values:", "Bitmask:"):
            pending = "values" if t == "Values:" else "bitmask"
        elif c.tag == "ul" and pending:
            p[pending] = list_pairs(c)
            pending = None
        elif t and not p["desc"] and pending is None:
            short.append(c.text())
    p["display"] = norm("".join(short))

    rng = norm(cells[2].text())
    if rng:
        m = _RANGE_OLD.match(rng) or None
        mi = _INCR_ONLY.match(rng)
        if m:
            set_range(p, m[1], m[2])
            p["increment"] = to_float(m[3])
        elif mi:
            p["increment"] = to_float(mi[1])
        else:
            warn(f"{name}: range '{rng}' not understood")
    set_default(p, cells[3].text(), warn)
    p["units"] = norm(cells[4].text()) or None
    return finish(p)


# ── v1.16 on: a heading per parameter ─────────────────────────────────────────

def parse_param_table(p, table, warn):
    head, rows = table_rows(table)
    if not rows:
        return
    cells = {h: norm(c.text()) for h, c in zip(head, rows[0])}
    p["reboot"]    = bool(cells.get("Reboot"))
    p["readonly"]  = bool(cells.get("Read-Only"))
    set_range(p, cells.get("minValue"), cells.get("maxValue"))
    p["increment"] = to_float(cells.get("increment"))
    set_default(p, cells.get("default", ""), warn)
    p["units"]     = cells.get("unit") or None
    for h in head:
        if h not in ("Reboot", "Read-Only", "minValue", "maxValue",
                     "increment", "default", "unit"):
            warn(f"{p['name']}: unknown column '{h}' ignored")


# ── Page ──────────────────────────────────────────────────────────────────────

_BLOCKS = {"h2", "h3", "p", "ul", "table"}


def blocks(node):
    """Block elements in document order, through any wrapping <div>."""
    for el in node.elems():
        if el.tag in _BLOCKS:
            yield el
        else:
            yield from blocks(el)


def parse_page(html, warn):
    params, group, cur, pending, had_table = [], None, None, None, False

    def close():
        nonlocal cur
        if cur is not None:
            params.append(finish(cur))
            cur = None

    for el in blocks(parse_html(html)):
        if el.tag == "h2":
            close()
            group = norm(el.text()).lstrip("#").strip()
        elif el.tag == "h3":
            close()
            name, typ = name_and_type(norm(el.text()))
            if name and el.attrs.get("id") == name:
                cur = new_record(name, group)
                cur["type"], cur["desc"], pending, had_table = typ, [], None, False
        elif el.tag == "table":
            head, rows = table_rows(el)
            if head[:2] == ["Name", "Description"]:
                close()
                for cells in rows:
                    if len(cells) >= 5:
                        p = parse_row(cells, group, warn)
                        if p: params.append(p)
            elif cur is not None and not had_table:
                parse_param_table(cur, el, warn)
                had_table = True
        elif cur is None or had_table:
            continue
        elif el.tag == "p":
            t = norm(el.text())
            if t in ("Values:", "Bitmask:"):
                pending = "values" if t == "Values:" else "bitmask"
            elif not cur["display"]:
                cur["display"] = t
            elif t:
                cur["desc"].append(t)
        elif el.tag == "ul" and pending:
            cur[pending] = list_pairs(el)
            pending = None
    close()
    return params


# PX4 says what it stores, so Int and Float are the page's word, not a guess.
# Values make an Enum only when they are the whole story (values_are_enum):
# GPS_1_PROTOCOL's range runs to 7 but its list stops at 5, so it stays a
# number, as does a float such as RC1_REV with its -1 and 1.
def classify(p):
    if p["bitmask"]:
        return "Bitmask"
    base = _TYPES.get(p["type"], "Float")
    if base == "Int" and values_are_enum(p["values"], p["range"]):
        return "Enum"
    return base


def releases(cache_dir):
    """[(major, minor, page url)] for every numbered release, oldest first."""
    html = cached(cache_dir, "main.html", MAIN_URL)
    out = set()
    for m in re.finditer(r'\\?"text\\?":\\?"v(\d+)\.(\d+)[^"\\]*\\?",\\?"link\\?":\\?"'
                         r'https://docs\.px4\.io/(v\d+\.\d+)/en/', html):
        out.add((int(m[1]), int(m[2]), SITE + m[3] + PAGE_PATH))
    if not out:
        sys.exit("no releases found in the version dropdown")
    return sorted(out)


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    out_path  = sys.argv[1]
    cache_dir = sys.argv[2] if len(sys.argv) == 3 else None
    if cache_dir:
        os.makedirs(cache_dir, exist_ok=True)

    versions = releases(cache_dir)
    warnings = set()
    pages = []
    for ma, mi, url in versions:
        params = parse_page(cached(cache_dir, f"v{ma}.{mi}.html", url), warnings.add)
        pages.append((f"v{ma}.{mi}", params))
        print(f"v{ma}.{mi}: {len(params)} parameters", file=sys.stderr)

    records = merge(pages, warnings.add)
    for w in sorted(warnings):
        print(f"warning: {w}", file=sys.stderr)

    first, last = versions[0], versions[-1]
    emit(records, [(ma, mi, 0) for ma, mi, _ in versions], out_path,
         script="gen_param_meta_px4.py",
         source=SITE + "v*" + PAGE_PATH,
         summary=f"{len(versions)} releases, v{first[0]}.{first[1]} to v{last[0]}.{last[1]}",
         symbol="param_docs_px4", display_name="PX4", minor_releases=True)
    print(f"{len(records)} records, {len({p['name'] for p, _ in records})} names "
          f"-> {out_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
