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

// lib/tile_fetch.cpp

#include "lib/tile_fetch.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

#include "httplib.h"

TileUrl tile_url_split(const std::string& url)
{
    const size_t scheme = url.find("://");
    const size_t slash  = url.find('/', scheme == std::string::npos ? 0 : scheme + 3);
    if (slash == std::string::npos) return { url, "/" };
    return { url.substr(0, slash), url.substr(slash) };
}

std::string tile_url_fill(std::string tmpl, int z, int x, int y)
{
    const auto sub = [&](const char* tok, int v) {
        const size_t n = std::strlen(tok);
        for (size_t p; (p = tmpl.find(tok)) != std::string::npos;)
            tmpl.replace(p, n, std::to_string(v));
    };
    sub("{z}", z); sub("{x}", x); sub("{y}", y);
    return tmpl;
}

std::string tile_cache_file(const std::string& dir, int z, int x, int y, const char* ext)
{
    return dir + "/" + std::to_string(z) + "/" + std::to_string(x)
         + "/" + std::to_string(y) + "." + ext;
}

std::vector<uint8_t> tile_disk_load(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return { std::istreambuf_iterator<char>(f), {} };
}

void tile_disk_save(const std::string& path, const void* data, size_t size)
{
    std::error_code ec;   // a folder that cannot be made shows up as the open failing
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream f(path, std::ios::binary);
    if (f) f.write(static_cast<const char*>(data), (std::streamsize)size);
}

std::unique_ptr<httplib::Client> tile_http_client(const std::string& host,
                                                  const std::string& user_agent)
{
    auto c = std::make_unique<httplib::Client>(host);
    c->set_connection_timeout(8);
    c->set_read_timeout(10);
    c->set_follow_location(true);
    c->set_default_headers({ { "User-Agent", user_agent } });
    return c;
}
