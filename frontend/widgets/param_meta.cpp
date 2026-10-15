/**************************************************************************
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


#include "param_meta.hpp"

#include <mavlink/ardupilotmega/mavlink.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>

int param_doc_version_for(const ParamDocSource& src, const char* fw_version)
{
    unsigned ma = 0, mi = 0, pa = 0;
    if (!fw_version || std::sscanf(fw_version, "%u.%u.%u", &ma, &mi, &pa) != 3)
        return -1;
    const uint32_t fw = (ma << 16) | (mi << 8) | pa;

    int best = 0;
    for (size_t i = 0; i < src.version_count; ++i) {
        const ParamDocVersion& v = src.versions[i];
        const uint32_t doc = ((uint32_t)v.major << 16) | ((uint32_t)v.minor << 8) | v.patch;
        if (doc <= fw) best = (int)i;
    }
    return best;
}

void param_doc_version_label(const ParamDocSource& src, int index, char* out, size_t n)
{
    if (index < 0 || (size_t)index >= src.version_count) { std::snprintf(out, n, "?"); return; }
    const ParamDocVersion& v = src.versions[index];
    if (src.minor_releases) std::snprintf(out, n, "v%u.%u", v.major, v.minor);
    else                    std::snprintf(out, n, "V%u.%u.%u", v.major, v.minor, v.patch);
}

// Releases between `version` and the nearest one in `mask`, and which that is.
// Ties go to the newer: its text is the more likely to still be right.
static int nearest_in(uint64_t mask, int version, int* at)
{
    int best = 1 << 30;
    for (int i = 0; i < 64; ++i) {
        if (!((mask >> i) & 1u)) continue;
        const int d = i > version ? i - version : version - i;
        if (d <= best) { best = d; *at = i; }
    }
    return best;
}

// The best record documented under exactly this name, or nullptr.
//
// Ranked by, in order: published for the vehicle's release; the heli variant
// for a heli and a non-heli one otherwise; how near its releases come to the
// vehicle's. Ties keep table order, which puts the generator's preferred
// variant first.
static const ParamMeta* find_exact(const ParamDocSource& src, const char* id, bool heli,
                                   int version, int* doc_version)
{
    const ParamMeta* tab = src.params;
    const ParamMeta* end = tab + src.param_count;

    const ParamMeta* lo = std::lower_bound(tab, end, id,
        [](const ParamMeta& m, const char* key) { return std::strcmp(m.name, key) < 0; });

    const ParamMeta* best = nullptr;
    int best_score[3] = {}, best_at = -1;
    for (const ParamMeta* m = lo; m != end && std::strcmp(m->name, id) == 0; ++m) {
        int at = -1;
        const int dist = nearest_in(m->versions, version, &at);
        const bool heli_variant = m->variant && std::strstr(m->variant, "Heli");
        const int score[3] = { dist != 0, heli_variant != heli, dist };
        if (!best || std::lexicographical_compare(score, score + 3,
                                                  best_score, best_score + 3)) {
            best = m;
            std::copy(score, score + 3, best_score);
            best_at = at;
        }
    }
    if (best && doc_version) *doc_version = best_at;
    return best;
}

const ParamMeta* get_param_meta(const ParamDocSource& src, const char* param_id,
                                bool heli, int version, int* doc_version)
{
    if (version < 0) version = (int)src.version_count - 1;

    if (const ParamMeta* m = find_exact(src, param_id, heli, version, doc_version)) return m;

    // Numbered instance no page lists: try each digit run as "1", "0" (PX4
    // counts from 0), then dropped — SERIAL9_BAUD → SERIAL1_BAUD,
    // CAL_ACC3_ID → CAL_ACC0_ID, BATT9_MONITOR → BATT_MONITOR.
    // One run at a time, so RC12_OPTION does not also renumber something else.
    const std::string id = param_id;
    for (size_t i = 0; i < id.size(); ) {
        if (!std::isdigit((unsigned char)id[i])) { ++i; continue; }
        size_t j = i;
        while (j < id.size() && std::isdigit((unsigned char)id[j])) ++j;

        for (const char* repl : { "1", "0", "" }) {
            const std::string alt = id.substr(0, i) + repl + id.substr(j);
            if (alt == id) continue;
            if (const ParamMeta* m = find_exact(src, alt.c_str(), heli, version, doc_version))
                return m;
        }
        i = j;
    }
    return nullptr;
}

static bool mav_type_is_int(uint8_t t)
{
    switch (t) {
    case MAV_PARAM_TYPE_UINT8:  case MAV_PARAM_TYPE_INT8:
    case MAV_PARAM_TYPE_UINT16: case MAV_PARAM_TYPE_INT16:
    case MAV_PARAM_TYPE_UINT32: case MAV_PARAM_TYPE_INT32:
    case MAV_PARAM_TYPE_UINT64: case MAV_PARAM_TYPE_INT64:
        return true;
    default:
        return false;
    }
}

ParamKind param_effective_kind(const ParamMeta* meta, uint8_t mav_param_type)
{
    if (meta && (meta->kind == ParamKind::Enum || meta->kind == ParamKind::Bitmask))
        return meta->kind;
    if (mav_type_is_int(mav_param_type))         return ParamKind::Int;
    if (mav_param_type == MAV_PARAM_TYPE_REAL32 ||
        mav_param_type == MAV_PARAM_TYPE_REAL64) return ParamKind::Float;
    return meta ? meta->kind : ParamKind::Float;
}

const char* param_kind_name(ParamKind k)
{
    switch (k) {
    case ParamKind::Float:   return "Float";
    case ParamKind::Int:     return "Integer";
    case ParamKind::Enum:    return "Enum";
    case ParamKind::Bitmask: return "Bitmask";
    }
    return "?";
}

const char* param_mav_type_name(uint8_t t)
{
    switch (t) {
    case MAV_PARAM_TYPE_UINT8:  return "uint8";
    case MAV_PARAM_TYPE_INT8:   return "int8";
    case MAV_PARAM_TYPE_UINT16: return "uint16";
    case MAV_PARAM_TYPE_INT16:  return "int16";
    case MAV_PARAM_TYPE_UINT32: return "uint32";
    case MAV_PARAM_TYPE_INT32:  return "int32";
    case MAV_PARAM_TYPE_UINT64: return "uint64";
    case MAV_PARAM_TYPE_INT64:  return "int64";
    case MAV_PARAM_TYPE_REAL32: return "float";
    case MAV_PARAM_TYPE_REAL64: return "double";
    default:                    return "unknown";
    }
}
