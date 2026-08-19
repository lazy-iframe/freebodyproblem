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


#include "param_file.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

// MAV_PARAM_TYPE values that carry a fraction. Everything else is an integer
// and is written as one — "0" rather than "0.000000000000000000", which is what
// the other ground stations produce and what a hand-edited file looks like.
static bool type_is_float(uint8_t type)
{
    return type == 9 /* REAL32 */ || type == 10 /* REAL64 */;
}

bool param_file_save(const std::string& path,
                     const std::vector<ParamFileRow>& rows,
                     const ParamFileHeader& header,
                     std::string* err)
{
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        if (err) *err = std::strerror(errno);
        return false;
    }

    f << "# Onboard parameters for Vehicle " << (int)header.sysid << "\n#\n";
    if (!header.stack.empty())    f << "# Stack: "        << header.stack    << "\n";
    if (!header.vehicle.empty())  f << "# Vehicle: "      << header.vehicle  << "\n";
    if (!header.version.empty())  f << "# Version: "      << header.version  << "\n";
    if (!header.git_hash.empty()) f << "# Git Revision: " << header.git_hash << "\n";
    f << "#\n# Vehicle-Id Component-Id Name Value Type\n";

    // snprintf rather than the stream: iostreams cannot express "18 decimals for
    // a float, none for an integer" without restating the format twice over.
    char line[128];
    for (const ParamFileRow& r : rows) {
        if (type_is_float(r.type)) {
            std::snprintf(line, sizeof(line), "%u\t%u\t%s\t%.18f\t%u\n",
                          (unsigned)r.sysid, (unsigned)r.compid, r.id.c_str(),
                          (double)r.value, (unsigned)r.type);
        } else {
            std::snprintf(line, sizeof(line), "%u\t%u\t%s\t%lld\t%u\n",
                          (unsigned)r.sysid, (unsigned)r.compid, r.id.c_str(),
                          (long long)r.value, (unsigned)r.type);
        }
        f << line;
    }

    f.flush();
    if (!f) {                      // disk full, read-only mount, …
        if (err) *err = "write failed";
        return false;
    }
    return true;
}

bool param_file_load(const std::string& path,
                     std::vector<ParamFileRow>* out,
                     int* skipped,
                     std::string* err)
{
    if (!out) return false;
    out->clear();
    if (skipped) *skipped = 0;

    std::ifstream f(path);
    if (!f) {
        if (err) *err = std::strerror(errno);
        return false;
    }

    std::string line;
    while (std::getline(f, line)) {
        // Tolerate CRLF: these files travel between Windows ground stations and
        // this one, and a stray '\r' would end up inside the type field.
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();

        // Comments and blank lines. Leading whitespace is not significant.
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos)    continue;
        if (line[first] == '#')            continue;

        // Whitespace-separated rather than strictly tab-separated: files edited
        // in a spreadsheet or by hand come back with spaces often enough.
        std::istringstream ls(line);
        long        sysid = 0, compid = 0, type = 0;
        std::string name;
        double      value = 0.0;
        if (!(ls >> sysid >> compid >> name >> value >> type)) {
            if (skipped) ++*skipped;
            continue;
        }
        if (name.empty() || name.size() > 16) {
            if (skipped) ++*skipped;
            continue;
        }

        ParamFileRow r;
        r.id     = name;
        r.value  = (float)value;
        r.type   = (uint8_t)type;
        r.sysid  = (uint8_t)sysid;
        r.compid = (uint8_t)compid;
        out->push_back(std::move(r));
    }

    return true;
}
