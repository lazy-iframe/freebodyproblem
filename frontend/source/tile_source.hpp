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


#pragma once

#include <cstdint>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Where the map's imagery comes from, and how a request to it is written.
//
// Holds no connection: the fetchers in components/map/ do the talking. Each
// provider has its own terms and asks for different things, and these differences
// are the reason this is more than a URL field:
//
//   Esri           works without a key today, though Esri's staff say the service
//                  needs a licence. With a token it goes to the endpoint Esri
//                  documents for that.
//   OpenStreetMap  no key, HTTPS only, must be cached, not for offline use.
//   Google         with a key, the Map Tiles API: a session made with the key, and a
//                  live credit. Without one, the unofficial endpoint other ground
//                  stations use, which Google's terms do not provide for and which
//                  could stop working at any time. A key that is set is never
//                  silently swapped for it. Tiles from the endpoint are cached like
//                  any other source's. The API's are not: its policy forbids storing
//                  them, and the key is the operator's account, agreed to those terms.
//   Sentinel-2     EOX's cloudless mosaic of Copernicus data: 10 m, no key, nothing
//                  past zoom 14. Recent years are CC BY-NC-SA: not for commercial use
//                  without a licence from EOX.
//   NASA GIBS      a chosen layer: true color, infrared or relief, no key, coarse. Some
//                  are per day, so they take a date. GIBS sometimes answers a good
//                  request with a blank tile, so the fetcher tries those again.
//   Custom         whatever the operator points it at.
//
// Most can serve more than one kind of data. Where a provider has no single layer for
// hybrid, it is two: a base, and an overlay the fetcher blends over it.
//
// Keys travel in the URL, so a request URL must never reach a log or a status line.
//
// Keyless Esri and Google, and custom URLs, get a desktop Firefox User-Agent for the
// platform, as other ground stations send. The keyed APIs, EOX and NASA get this app's
// name. So does OpenStreetMap on any URL: its tile policy forbids impersonating a browser.
// ─────────────────────────────────────────────────────────────────────────────

// Saved in settings by number, so the numbering is fixed.
enum class TileProvider : int { Esri = 0, OpenStreetMap = 1, Google = 2, Custom = 3, Sentinel2 = 4, NasaGibs = 5 };
constexpr int TILE_PROVIDER_COUNT = 6;

enum class TileKind : int { Satellite = 0, Hybrid = 1, Terrain = 2 };
constexpr int TILE_KIND_COUNT = 3;

// Names this app, its version and where to find it. OpenStreetMap's tile policy asks for
// exactly that, and blocks library defaults.
constexpr char GCS_USER_AGENT[] =
    "freebodyproblem-gcs/" GCS_VERSION " (+https://github.com/lazy-iframe/freebodyproblem)";

constexpr char TILE_ESRI_URL[] =
    "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
constexpr char TILE_ESRI_KEYED_URL[] =
    "https://ibasemaps-api.arcgis.com/arcgis/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
constexpr char TILE_ESRI_ATTRIBUTION[] =
    "Source: Esri, Vantor, Earthstar Geographics, and the GIS User Community";
// What the fetcher blends for hybrid, and what Terrain uses. Not World_Terrain_Base:
// outside the US that answers "Map data not yet available" from about zoom 11, which
// is exactly where a mission is planned. The keyed host has neither of these, so a key
// only applies to satellite data.
constexpr char TILE_ESRI_PLACES_URL[] =
    "https://server.arcgisonline.com/ArcGIS/rest/services/Reference/World_Boundaries_and_Places/MapServer/tile/{z}/{y}/{x}";
constexpr char TILE_ESRI_TOPO_URL[] =
    "https://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer/tile/{z}/{y}/{x}";
constexpr char TILE_SENTINEL_URL[] =
    "https://tiles.maps.eox.at/wmts/1.0.0/s2cloudless-2024_3857/default/GoogleMapsCompatible/{z}/{y}/{x}.jpg";
constexpr char TILE_SENTINEL_STREETS_URL[] =
    "https://tiles.maps.eox.at/wmts/1.0.0/streets_3857/default/GoogleMapsCompatible/{z}/{y}/{x}.png";
constexpr char TILE_SENTINEL_TERRAIN_URL[] =
    "https://tiles.maps.eox.at/wmts/1.0.0/terrain_3857/default/g/{z}/{y}/{x}.jpg";
constexpr int  TILE_SENTINEL_MAX_ZOOM = 14;
constexpr int  TILE_DEFAULT_MAX_ZOOM  = 22;
constexpr char TILE_OSM_URL[]         = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
constexpr char TILE_OSM_ATTRIBUTION[] = "\xc2\xa9 OpenStreetMap contributors";
constexpr char TILE_GOOGLE_HOST[]     = "https://tile.googleapis.com";
constexpr char TILE_GOOGLE_NAME[]     = "Google Maps";
constexpr char TILE_GOOGLE_KEYLESS_URL[] = "https://mt{s}.google.com/vt/lyrs=s&x={x}&y={y}&z={z}";

struct TileSourceConfig {
    TileProvider provider = TileProvider::Esri;
    TileKind     kind     = TileKind::Satellite;   // for those that have a choice
    int          gibs_layer = 0;           // into tile_gibs_layers()
    std::string  gibs_date;                // YYYY-MM-DD, for a layer that takes one
    std::string  esri_key;
    std::string  google_key;
    std::string  custom_url;           // {z} {x} {y}, and {s} and {key} if it takes them
    std::string  custom_key;
    std::string  custom_attribution;

    bool operator==(const TileSourceConfig& o) const
    {
        return provider == o.provider && kind == o.kind && gibs_layer == o.gibs_layer &&
               gibs_date == o.gibs_date && esri_key == o.esri_key &&
               google_key == o.google_key && custom_url == o.custom_url &&
               custom_key == o.custom_key && custom_attribution == o.custom_attribution;
    }
    bool operator!=(const TileSourceConfig& o) const { return !(*this == o); }
};

const char* tile_provider_label(TileProvider p);
const char* tile_kind_label(TileKind k);

// Whether it offers satellite, hybrid and terrain. OpenStreetMap is one street map, a
// custom URL is what it is, and NASA GIBS has its own list of layers instead.
bool tile_provider_has_kinds(TileProvider p);

// NASA GIBS's layers. Saved in settings by position, so only ever add to the end.
struct TileGibsLayer {
    const char* id;
    const char* label;
    const char* credit;
    int         max_zoom;
    const char* extension;
    bool        dated;      // per day, so the date picks the imagery
};
int                  tile_gibs_layer_count();
const TileGibsLayer& tile_gibs_layer(int index);   // an index out of range is the first

// Dates as YYYY-MM-DD, by the calendar. Today is UTC, as the data is.
bool        tile_date_valid(const std::string& date);
std::string tile_date_add(const std::string& date, int days);
std::string tile_date_today();

// The zoom past which it has no tiles, so the map stretches this one instead.
int tile_source_max_zoom(const TileSourceConfig& c);

// What the operator should know before choosing it. One sentence or two.
const char* tile_provider_note(TileProvider p);

// Why this source cannot be used as configured, or nullptr if it can.
const char* tile_source_problem(const TileSourceConfig& c);

// The User-Agent for requests to this host, written as "https://name" or as a bare
// name. By host and not by provider, so a connection kept per host always carries the
// right one. A custom URL on a host named here is treated as that host, not as custom.
const char* tile_user_agent(const std::string& host);

// Percent-encoded, so a key cannot break out of its place in a URL.
std::string tile_url_encode(const std::string& s);

// The full URL with {z} {x} {y}, and {s} for those with numbered servers, left in it.
// For Google with a key, `google_session` is the token from createSession; the
// others ignore it.
std::string tile_source_template(const TileSourceConfig& c, const std::string& google_session);

// What to blend over the base for hybrid, in the same form; empty if nothing is, and
// its own cache folder. A failure of the overlay leaves the base showing.
std::string tile_source_overlay_template(const TileSourceConfig& c);
std::string tile_source_overlay_cache_dir(const TileSourceConfig& c);

// The folder under the tile cache, so one provider's tiles are never shown as
// another's. False when this provider's tiles must not be written to disk at all.
// Esri's is the top of the cache, where its tiles have always been.
bool tile_source_cache_dir(const TileSourceConfig& c, std::string& subdir);

// The credit to show, without any live part Google adds to it.
std::string tile_source_attribution(const TileSourceConfig& c);

// Whether this source goes through Google's session and live credit.
inline bool tile_source_uses_google_api(const TileSourceConfig& c)
{
    return c.provider == TileProvider::Google && !c.google_key.empty();
}

// Google's createSession, its URL and its body.
std::string tile_google_session_path(const std::string& key);
std::string tile_google_session_body(TileKind kind);

// Reads createSession's reply. `expiry` is seconds since the epoch. False if the
// reply is not one, whatever it says.
bool tile_google_parse_session(const std::string& body, std::string& session, int64_t& expiry);

// Google's viewport call, which says what the imagery in that area must be credited
// with. The reading of its reply is empty if it did not say.
std::string tile_google_viewport_path(const std::string& session, const std::string& key, int zoom,
                                      double north, double south, double east, double west);
std::string tile_google_parse_copyright(const std::string& body);
