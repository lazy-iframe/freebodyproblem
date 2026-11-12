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

// lib/tile_fetch.hpp
//
// The plumbing every slippy-map tile fetch shares: the map imagery
// (frontend/components/map/tiles.cpp) and the terrain heights
// (frontend/source/terrain.cpp) are both z/x/y tiles from a URL template,
// cached on disk under the same layout and fetched over the same kind of
// connection. What each does with the bytes, and its queue and workers, stay
// with it.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace httplib { class Client; }

// "https://host/path/{z}/{x}/{y}.png" as the host, scheme included, and the
// path template: "https://host" and "/path/{z}/{x}/{y}.png". Without a scheme
// the host is everything up to the first '/', which the HTTP client takes as
// plain http. A URL with no path gets "/".
struct TileUrl {
    std::string host;
    std::string path;
};
TileUrl tile_url_split(const std::string& url);

// The template with every {z}, {x} and {y} filled in.
std::string tile_url_fill(std::string tmpl, int z, int x, int y);

// Where a tile is cached: <dir>/<z>/<x>/<y>.<ext>.
std::string tile_cache_file(const std::string& dir, int z, int x, int y, const char* ext);

// The cached bytes, or empty when there are none.
std::vector<uint8_t> tile_disk_load(const std::string& path);

// Best effort, creating the folders on the way: a tile that cannot be saved is
// fetched again next time, which is all a cache miss costs.
void tile_disk_save(const std::string& path, const void* data, size_t size);

// A connection to one tile host: 8 s to connect, 10 s to read, redirects
// followed, and the given User-Agent, which tile servers' usage policies ask
// an app to identify itself by.
std::unique_ptr<httplib::Client> tile_http_client(const std::string& host,
                                                  const std::string& user_agent);
