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
What the parameter-reference generators share: a small HTML tree, the page
cache, the merge of many releases into one table, and the C++ it is written
out as. Each firmware's script — gen_param_meta_ardupilot.py (Copter, Plane),
gen_param_meta_px4.py — only knows how to read its own pages.

A parameter record is a dict with the keys in CONTENT_KEYS:

    name, variant      the ID, and the owning library where a page documents
                       one name several times (ArduPilot only), else None
    display, group     short title; the page section it sits in
    desc               long description, paragraphs joined by blank lines
    reboot, advanced, readonly, calibration, volatile    bools
    units              str or None
    range              (min, max) or None
    increment          float or None
    default            float or None
    values, bitmask    ((value, label), ...) / ((bit, label), ...) or None
    kind               "Float" | "Int" | "Enum" | "Bitmask"
"""

import datetime
import math
import os
import re
import sys
import urllib.request
from html.parser import HTMLParser

LICENSE_HEADER = """/**************************************************************************
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

CONTENT_KEYS = ("name", "variant", "display", "group", "desc", "reboot",
                "advanced", "readonly", "calibration", "volatile", "units",
                "range", "increment", "default", "values", "bitmask", "kind")


def new_record(name, group):
    return {
        "name": name, "variant": None, "display": "", "group": group,
        "desc": "", "reboot": False, "advanced": False, "readonly": False,
        "calibration": False, "volatile": False,
        "units": None, "range": None, "increment": None, "default": None,
        "values": None, "bitmask": None, "kind": "Float",
    }


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

    def has_class(self, cls):
        return cls in self.attrs.get("class", "").split()


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


def parse_html(html):
    tb = TreeBuilder()
    tb.feed(html)
    return tb.root


def norm(s):
    return " ".join(s.split())


def table_rows(table):
    """Header cells and data rows of a table, cells kept as Nodes."""
    head, rows = [], []
    for part in table.elems():
        for tr in part.elems("tr"):
            cells = tr.elems("th") + tr.elems("td")
            if part.tag == "thead":
                head = [norm(c.text()) for c in cells]
            else:
                rows.append(cells)
    return head, rows


def is_int(x):
    return float(x).is_integer()


# An open range end — PX4's "[0, ?]" — is stored as the float limit, which no
# value passes and the GCS prints as "no limit".
FLT_MAX = 3.4028234663852886e38


def values_are_enum(values, rng):
    """Whether a Values list is the whole story, so a dropdown loses nothing.

    It is when every value is whole and the documented range admits no value
    the list leaves out. Otherwise the values are landmarks on a continuum —
    ATC_INPUT_TC's 0.5 "Very Soft", 0.2 "Soft", ... anywhere in 0 to 1; a
    *_PIN's -1 "Disabled", 50 "AUX1", ... with every pin between valid — and
    the parameter is a number, its values only suggestions.
    """
    if not values or not all(is_int(v) for v, _ in values):
        return False
    if rng is None:
        return True
    listed = {int(v) for v, _ in values}
    lo = min(listed) if rng[0] <= -FLT_MAX else rng[0]
    hi = max(listed) if rng[1] >= FLT_MAX else rng[1]
    if hi - lo > 4096:
        return False
    return all(i in listed for i in range(math.ceil(lo), math.floor(hi) + 1))


def to_float(s):
    try:
        return float(s)
    except (TypeError, ValueError):
        return None


# Anything the GCS cannot use as a bit or a value is dropped rather than
# guessed at; the rest of the record still shows in the info panel.
def clean_bits(pairs):
    return tuple((int(b), l) for b, l in pairs or ()
                 if str(b).isdigit() and int(b) < 32) or None


def clean_values(pairs):
    out = []
    for v, l in pairs or ():
        f = to_float(v)
        if f is not None:
            out.append((f, l))
    return tuple(out) or None


# ── Fetching ──────────────────────────────────────────────────────────────────

def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": "freebodyproblem-gen"})
    return urllib.request.urlopen(req).read().decode("utf-8")


def cached(cache_dir, name, url):
    """`url`'s text, from `cache_dir`/`name` when an earlier run saved it."""
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


# ── Merge ─────────────────────────────────────────────────────────────────────

def merge(pages, warn):
    """[(version label, [record])] oldest first → [(record, release mask)].

    A record whose content is identical across releases is kept once, its
    mask the set of releases that publish it so.
    """
    if len(pages) > 64:
        sys.exit("more than 64 releases: widen ParamMeta::versions")
    merged = {}
    for idx, (label, params) in enumerate(pages):
        seen = {}
        for p in params:
            key     = (p["name"], p["variant"])
            content = tuple(p[k] for k in CONTENT_KEYS)
            # Some pages print a parameter twice; an exact repeat is harmless,
            # a differing one worth knowing about.
            if key in seen:
                if seen[key] != content:
                    warn(f"{label}: {p['name']} listed twice with different text, kept first")
                continue
            seen[key] = content
            merged[content] = merged.get(content, 0) | (1 << idx)
    return [(dict(zip(CONTENT_KEYS, c)), mask) for c, mask in merged.items()]


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


def emit(records, versions, out_path, *, script, source, summary, symbol,
         display_name, minor_releases, preferred_variants=()):
    """Write the C++ table.

    versions        [(major, minor, patch)], oldest first, bit i of a mask = entry i
    script, source  the banner's generator name and source URL
    summary         the banner's line describing which releases these are
    symbol          the ParamDocSource the file defines
    display_name    ParamDocSource::name, "Copter", "PX4"
    minor_releases  one page per minor release: labelled "v1.15", not "V4.5.7"
    """
    strings = Pool("s")
    enums   = Pool("E")
    bits    = Pool("B")

    def s(x):
        return strings.ref(x, lambda i: f"const char {i}[] = {c_str(x)};")

    def newest(mask):
        return mask.bit_length() - 1

    # By name in strcmp order, for the lookup's binary search; within a name
    # the preferred variant first, then the newest text.
    records = sorted(records, key=lambda r: (r[0]["name"].encode(),
                                             r[0]["variant"] not in preferred_variants,
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
        if p["default"] is not None:   flags.append("PARAM_FLAG_HAS_DEFAULT")
        rng = p["range"] or (0.0, 0.0)
        rows.append(
            f"    {{ {s(p['name'])}, {s(p['variant'])}, {s(p['display'])}, {s(p['group'])}, "
            f"{s(p['desc'])}, {s(p['units'])}, "
            f"{c_float(rng[0])}, {c_float(rng[1])}, {c_float(p['increment'] or 0.0)}, "
            f"{c_float(p['default'] or 0.0)}, "
            f"{' | '.join(flags) or '0'}, ParamKind::{p['kind']}, "
            f"{e}, {len(p['values'] or ())}, {b}, {len(p['bitmask'] or ())}, "
            f"0x{mask:016x}ull }},")

    lines = [LICENSE_HEADER,
             f"// GENERATED by scripts/{script} - do not edit by hand.",
             f"// Source: {source} ({datetime.date.today().isoformat()})",
             f"// {summary}; "
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
    lines += [f"    {{ {a}, {b}, {c} }}," for a, b, c in versions]
    lines += ["};", ""]
    # Rows reference the pools, so the pools are filled first and written out
    # ahead of the rows.
    lines += strings.lines + [""] + enums.lines + [""] + bits.lines
    lines += ["", "const ParamMeta kParams[] = {"] + rows + [
        "};",
        "",
        "} // namespace",
        "",
        f"const ParamDocSource {symbol} = {{",
        f"    {c_str(display_name)},",
        "    kParams,   std::size(kParams),",
        "    kVersions, std::size(kVersions),",
        f"    {'true' if minor_releases else 'false'},",
        "};",
        "",
    ]
    with open(out_path, "w", encoding="ascii", newline="\n") as f:
        f.write("\n".join(lines))
    return records
