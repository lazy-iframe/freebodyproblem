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
Generate param_meta_generated.cpp from ArduPilot's published Copter parameter
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

Usage: gen_param_meta.py <out_cpp> [<cache_dir>]

<cache_dir> keeps the downloaded pages, and pages already in it are not
fetched again.
"""

import datetime
import json
import os
import re
import sys
import urllib.request
from html.parser import HTMLParser

BASE_URL     = "https://ardupilot.org/copter/"
VERSIONS_URL = BASE_URL + "_static/parameters-Copter.json"
PAGE_URL     = BASE_URL + "docs/"

_LICENSE_HEADER = """/**************************************************************************
 * Copyright (C) 2026  Azhar Tanweer
 * Contact: azhar.tanweer404@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************/
"""

# ── Minimal DOM ───────────────────────────────────────────────────────────────
#
# Standard library only, so regenerating needs nothing a stock Python lacks.

_VOID = {"br", "img", "hr", "meta", "link", "input", "col", "wbr", "source"}


class Node:
    __slots__ = ("tag", "attrs", "children", "parent")

    def __init__(self, tag, attrs, parent):
        self.tag, self.attrs, self.children, self.parent = tag, dict(attrs), [], parent

    def text(self):
        out = []
        for c in self.children:
            out.append(c if isinstance(c, str) else c.text())
        return "".join(out)

    def elems(self, tag=None):
        return [c for c in self.children
                if isinstance(c, Node) and (tag is None or c.tag == tag)]

    def find_all(self, tag):
        for c in self.children:
            if isinstance(c, Node):
                if c.tag == tag:
                    yield c
                yield from c.find_all(tag)


class TreeBuilder(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.root = Node("#root", {}, None)
        self.cur = self.root

    def handle_starttag(self, tag, attrs):
        n = Node(tag, attrs, self.cur)
        self.cur.children.append(n)
        if tag not in _VOID:
            self.cur = n

    def handle_endtag(self, tag):
        n = self.cur
        while n is not None and n.tag != tag:
            n = n.parent
        if n is not None and n.parent is not None:
            self.cur = n.parent

    def handle_data(self, data):
        self.cur.children.append(data)


def norm(s):
    return " ".join(s.split())


# ── Page → parameter records ──────────────────────────────────────────────────

def heading_of(section):
    for tag in ("h1", "h2", "h3"):
        h = section.elems(tag)
        if h:
            return tag, norm(h[0].text()).rstrip("¶").strip()
    return None, None


def table_rows(table):
    """Header cells and data rows of a docutils table, nested tables kept as Nodes."""
    head, rows = [], []
    for part in table.elems():
        for tr in part.elems("tr"):
            cells = tr.elems("th") + tr.elems("td")
            if part.tag == "thead":
                head = [norm(c.text()) for c in cells]
            else:
                rows.append(cells)
    return head, rows


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
    p = {
        "name": name, "variant": variant.rstrip(")") or None,
        "display": display.strip(), "group": group,
        "desc": [], "reboot": False, "advanced": False, "readonly": False,
        "calibration": False, "volatile": False,
        "units": None, "range": None, "increment": None,
        "values": None, "bitmask": None,
    }
    for el in section.elems():
        if el.tag == "div" and "line-block" in el.attrs.get("class", ""):
            note = norm(el.text()).lower()
            if "reboot required" in note: p["reboot"] = True
            if "advanced users" in note:  p["advanced"] = True
        elif el.tag == "p":
            t = norm(el.text())
            if t: p["desc"].append(t)
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
    p["desc"] = "\n\n".join(p["desc"])

    # Anything the GCS cannot use as a bit or a value is dropped rather than
    # guessed at; what remains still shows in the info panel.
    if p["bitmask"]:
        p["bitmask"] = tuple((int(b), l) for b, l in p["bitmask"]
                             if b.isdigit() and int(b) < 32) or None
    if p["values"]:
        good = []
        for v, l in p["values"]:
            try: good.append((float(v), l))
            except ValueError: pass
        p["values"] = tuple(good) or None
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
    tb = TreeBuilder()
    tb.feed(html)
    params = []
    walk(tb.root, None, params, warn)
    return params


def is_int(x):
    return float(x).is_integer()


# What the page says each parameter is. It never names a storage type — that
# only the vehicle knows, in PARAM_VALUE's param_type — so a plain number is
# called an integer when everything the page does say about it is whole:
# its step, both ends of its range. The GCS prefers the vehicle's word over
# this guess wherever it has one.
#
# A Values list is an enum only when it is the whole story. Alongside a Range
# it names landmarks on a continuum — ATC_INPUT_TC's 0.5 "Very Soft", 0.2
# "Soft", ... anywhere in 0 to 1; a *_PIN's -1 "Disabled", 50 "AUX1", ... —
# and the parameter stays a number, its values kept as suggestions.
def classify(p):
    if p["bitmask"]:
        return "Bitmask"
    if p["values"] and not p["range"] and all(is_int(v) for v, _ in p["values"]):
        return "Enum"
    inc, rng = p["increment"], p["range"]
    if inc is not None and is_int(inc) and (rng is None or (is_int(rng[0]) and is_int(rng[1]))):
        return "Int"
    return "Float"


# ── Fetching ──────────────────────────────────────────────────────────────────

def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": "freebodyproblem-gen"})
    return urllib.request.urlopen(req).read().decode("utf-8")


def cached(cache_dir, name, url):
    if cache_dir:
        path = os.path.join(cache_dir, name)
        if os.path.exists(path):
            with open(path, encoding="utf-8") as f:
                return f.read()
    text = fetch(url)
    if cache_dir:
        with open(os.path.join(cache_dir, name), "w", encoding="utf-8") as f:
            f.write(text)
    return text


def stable_versions(cache_dir):
    """[(major, minor, patch, page)] for every stable release, oldest first."""
    picker = json.loads(cached(cache_dir, "parameters-Copter.json", VERSIONS_URL))
    out = []
    for label, page in picker.items():
        m = re.fullmatch(r"Copter stable V(\d+)\.(\d+)\.(\d+)", label)
        if m:
            out.append((int(m[1]), int(m[2]), int(m[3]), page))
    out.sort()
    if len(out) > 64:
        sys.exit("more than 64 stable releases: widen ParamMeta::versions")
    return out


# ── Emit ──────────────────────────────────────────────────────────────────────

# MSVC caps a single string literal token at 16 KiB; long descriptions are
# split into adjacent literals the compiler joins back together. Escaped a
# character at a time so a split never lands inside an escape, and non-ASCII
# as octal, which unlike \x cannot swallow a following hex-looking letter.
def c_str(s):
    if s is None:
        return "nullptr"
    toks = []
    for ch in s:
        if ch == "\\":   toks.append("\\\\")
        elif ch == '"':  toks.append('\\"')
        elif ch == "\n": toks.append("\\n")
        elif 32 <= ord(ch) < 127: toks.append(ch)
        else: toks.extend(f"\\{b:03o}" for b in ch.encode())
    chunks, cur = [], ""
    for t in toks:
        if len(cur) + len(t) > 2000:
            chunks.append(cur); cur = ""
        cur += t
    chunks.append(cur)
    return " ".join(f'"{c}"' for c in chunks)


def c_float(x):
    r = repr(float(x))
    if "e" not in r and "." not in r:
        r += ".0"
    return r + "f"


# Of a parameter's variants, the one a copter is most likely to be running:
# sorted first, so a lookup that knows nothing more lands on it.
PREFERRED_VARIANTS = {
    "AC_AttitudeControl_Multi", "AC_AutoTune_Multi",
    "AP_BattMonitor_Analog", "AP_MotorsHeli_Single",
}

CONTENT_KEYS = ("name", "variant", "display", "group", "desc", "reboot",
                "advanced", "readonly", "calibration", "volatile", "units",
                "range", "increment", "values", "bitmask")


class Pool:
    """Each distinct string or table emitted once, referred to by name."""
    def __init__(self, prefix):
        self.prefix, self.ids, self.lines = prefix, {}, []

    def ref(self, key, render):
        if key is None:
            return "nullptr"
        if key not in self.ids:
            ident = f"{self.prefix}{len(self.ids)}"
            self.ids[key] = ident
            self.lines.append(render(ident))
        return self.ids[key]


def emit(records, versions, out_path):
    strings = Pool("s")
    enums   = Pool("E")
    bits    = Pool("B")

    def s(x):
        return strings.ref(x, lambda i: f"const char {i}[] = {c_str(x)};")

    def newest(mask):
        return mask.bit_length() - 1

    records = sorted(records, key=lambda r: (r[0]["name"].encode(),
                                             r[0]["variant"] not in PREFERRED_VARIANTS,
                                             r[0]["variant"] or "",
                                             -newest(r[1])))
    rows = []
    for p, mask in records:
        e = enums.ref(p["values"], lambda i: f"const ParamEnumEntry {i}[] = {{ " + ", ".join(
            f"{{ {c_float(v)}, {s(l)} }}" for v, l in p["values"]) + " };")
        b = bits.ref(p["bitmask"], lambda i: f"const ParamBitmaskBit {i}[] = {{ " + ", ".join(
            f"{{ {bit}, {s(l)} }}" for bit, l in p["bitmask"]) + " };")
        flags = [f for f, k in (("PARAM_FLAG_REBOOT", "reboot"),
                                ("PARAM_FLAG_ADVANCED", "advanced"),
                                ("PARAM_FLAG_READONLY", "readonly"),
                                ("PARAM_FLAG_CALIBRATION", "calibration"),
                                ("PARAM_FLAG_VOLATILE", "volatile")) if p[k]]
        if p["range"]:                 flags.append("PARAM_FLAG_HAS_RANGE")
        if p["increment"] is not None: flags.append("PARAM_FLAG_HAS_INCREMENT")
        rng = p["range"] or (0.0, 0.0)
        rows.append(
            f"    {{ {s(p['name'])}, {s(p['variant'])}, {s(p['display'])}, {s(p['group'])}, "
            f"{s(p['desc'])}, {s(p['units'])}, "
            f"{c_float(rng[0])}, {c_float(rng[1])}, {c_float(p['increment'] or 0.0)}, "
            f"{' | '.join(flags) or '0'}, ParamKind::{classify(p)}, "
            f"{e}, {len(p['values'] or ())}, {b}, {len(p['bitmask'] or ())}, "
            f"0x{mask:016x}ull }},")

    first, last = versions[0], versions[-1]
    lines = [_LICENSE_HEADER,
             "// GENERATED by scripts/gen_param_meta.py - do not edit by hand.",
             f"// Source: {PAGE_URL}parameters-Copter-stable-V*.html "
             f"({datetime.date.today().isoformat()})",
             f"// {len(versions)} stable releases, V{first[0]}.{first[1]}.{first[2]} to "
             f"V{last[0]}.{last[1]}.{last[2]}; "
             f"{len({p['name'] for p, _ in records})} parameter names, "
             f"{len(records)} distinct records.",
             "",
             '#include "param_meta.hpp"',
             "",
             "#include <iterator>",
             "",
             "namespace {",
             "",
             "// Bit i of ParamMeta::versions is this table's entry i.",
             "const ParamDocVersion kVersions[] = {"]
    lines += [f"    {{ {a}, {b}, {c} }}," for a, b, c, _ in versions]
    lines += ["};", ""]
    # Rows reference the pools, so the pools are filled first and written out
    # ahead of the rows.
    lines += strings.lines + [""] + enums.lines + [""] + bits.lines
    lines += ["", "const ParamMeta kParams[] = {"] + rows + [
        "};",
        "",
        "} // namespace",
        "",
        "const ParamMeta* param_meta_table(size_t* count)",
        "{",
        "    *count = std::size(kParams);",
        "    return kParams;",
        "}",
        "",
        "const ParamDocVersion* param_doc_versions(size_t* count)",
        "{",
        "    *count = std::size(kVersions);",
        "    return kVersions;",
        "}",
        "",
    ]
    with open(out_path, "w", encoding="ascii", newline="\n") as f:
        f.write("\n".join(lines))
    return records


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    out_path  = sys.argv[1]
    cache_dir = sys.argv[2] if len(sys.argv) == 3 else None
    if cache_dir:
        os.makedirs(cache_dir, exist_ok=True)

    versions = stable_versions(cache_dir)
    warnings = set()

    # (content) -> bitmask of the releases documenting it exactly so
    merged = {}
    for idx, (ma, mi, pa, page) in enumerate(versions):
        html = cached(cache_dir, page, PAGE_URL + page)
        params = parse_page(html, warnings.add)
        seen = {}
        for p in params:
            key     = (p["name"], p["variant"])
            content = tuple(p[k] for k in CONTENT_KEYS)
            # Some pages print a parameter twice (4.7's LightWare GRF entries);
            # an exact repeat is harmless, a differing one worth knowing about.
            if key in seen:
                if seen[key] != content:
                    warnings.add(f"V{ma}.{mi}.{pa}: {p['name']} listed twice "
                                 "with different text, kept first")
                continue
            seen[key] = content
            merged[content] = merged.get(content, 0) | (1 << idx)
        print(f"V{ma}.{mi}.{pa}: {len(params)} parameters", file=sys.stderr)

    for w in sorted(warnings):
        print(f"warning: {w}", file=sys.stderr)

    records = [(dict(zip(CONTENT_KEYS, c)), mask) for c, mask in merged.items()]
    emit(records, versions, out_path)
    print(f"{len(records)} records, {len({p['name'] for p, _ in records})} names "
          f"-> {out_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
