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


#include "tile_source.hpp"

#include <nlohmann/json.hpp>

#include <cctype>
#include <cstdio>
#include <ctime>
#include <exception>

using json = nlohmann::json;

const char* tile_provider_label(TileProvider p)
{
    switch (p) {
    case TileProvider::Esri:          return "Esri World Imagery";
    case TileProvider::OpenStreetMap: return "OpenStreetMap";
    case TileProvider::Google:        return "Google Satellite";
    case TileProvider::Sentinel2:     return "Sentinel-2 Cloudless (10 m)";
    case TileProvider::NasaGibs:      return "NASA GIBS (infrared, HLS, Landsat, ASTER)";
    case TileProvider::Custom:        return "Custom URL";
    }
    return "";
}

const char* tile_kind_label(TileKind k)
{
    switch (k) {
    case TileKind::Satellite: return "SATELLITE";
    case TileKind::Hybrid:    return "HYBRID";
    case TileKind::Terrain:   return "TERRAIN";
    }
    return "";
}

bool tile_provider_has_kinds(TileProvider p)
{
    return p == TileProvider::Esri || p == TileProvider::Google || p == TileProvider::Sentinel2;
}

static const TileGibsLayer GIBS_LAYERS[] = {
    { "HLS_S30_Nadir_BRDF_Adjusted_Reflectance", "HLS Sentinel-2, true color, 30 m (by date)",
      "HLS S30", 12, "png", true },
    { "Landsat_WELD_CorrectedReflectance_TrueColor_Global_Annual", "Landsat, true color, 30 m (around 2000)",
      "Landsat WELD", 12, "jpg", false },
    { "Landsat_WELD_CorrectedReflectance_Bands743_Global_Annual", "Landsat infrared, bands 7-4-3, 30 m (around 2000)",
      "Landsat WELD bands 7-4-3", 12, "jpg", false },
    { "Landsat_WELD_CorrectedReflectance_Bands157_Global_Annual", "Landsat infrared, bands 1-5-7, 30 m (around 2000)",
      "Landsat WELD bands 1-5-7", 12, "jpg", false },
    { "VIIRS_NOAA20_CorrectedReflectance_BandsM11-I2-I1", "VIIRS shortwave infrared, M11-I2-I1 (daily)",
      "VIIRS NOAA-20 bands M11-I2-I1", 9, "jpg", true },
    { "VIIRS_NOAA20_CorrectedReflectance_BandsM3-I3-M11", "VIIRS snow and cloud, M3-I3-M11 (daily)",
      "VIIRS NOAA-20 bands M3-I3-M11", 9, "jpg", true },
    { "VIIRS_NOAA20_Brightness_Temp_BandI5_Day", "VIIRS thermal infrared, band I5, day (daily)",
      "VIIRS NOAA-20 brightness temperature, band I5", 9, "png", true },
    { "ASTER_GDEM_Color_Shaded_Relief", "ASTER shaded relief",
      "ASTER GDEM", 12, "jpg", false },
};

int tile_gibs_layer_count() { return (int)(sizeof(GIBS_LAYERS) / sizeof(GIBS_LAYERS[0])); }

const TileGibsLayer& tile_gibs_layer(int index)
{
    return GIBS_LAYERS[(index >= 0 && index < tile_gibs_layer_count()) ? index : 0];
}

// Days since 1970-01-01 and back, by Howard Hinnant's civil calendar formulas.
static int64_t days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153u * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int64_t)doe - 719468;
}

static std::string civil_from_days(int64_t z)
{
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp  = (5 * doy + 2) / 153;
    const unsigned d   = doy - (153 * mp + 2) / 5 + 1;
    const unsigned m   = mp < 10 ? mp + 3 : mp - 9;
    const int      y   = (int)yoe + (int)(era * 400) + (m <= 2);
    char out[16];
    snprintf(out, sizeof(out), "%04d-%02u-%02u", y, m, d);
    return out;
}

static bool parse_date(const std::string& s, int& y, int& m, int& d)
{
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (int i : { 0, 1, 2, 3, 5, 6, 8, 9 })
        if (s[i] < '0' || s[i] > '9') return false;
    y = std::stoi(s.substr(0, 4));
    m = std::stoi(s.substr(5, 2));
    d = std::stoi(s.substr(8, 2));
    if (m < 1 || m > 12 || d < 1) return false;
    return civil_from_days(days_from_civil(y, m, d)) == s;   // rejects 02-30 and the like
}

bool tile_date_valid(const std::string& date)
{
    int y, m, d;
    return parse_date(date, y, m, d);
}

std::string tile_date_add(const std::string& date, int days)
{
    int y, m, d;
    if (!parse_date(date, y, m, d)) return date;
    return civil_from_days(days_from_civil(y, m, d) + days);
}

std::string tile_date_today()
{
    return civil_from_days((int64_t)(std::time(nullptr) / 86400));
}

int tile_source_max_zoom(const TileSourceConfig& c)
{
    if (c.provider == TileProvider::Sentinel2) return TILE_SENTINEL_MAX_ZOOM;
    if (c.provider == TileProvider::NasaGibs)  return tile_gibs_layer(c.gibs_layer).max_zoom;
    return TILE_DEFAULT_MAX_ZOOM;
}

const char* tile_provider_note(TileProvider p)
{
    switch (p) {
    case TileProvider::Esri:
        return "A key applies to satellite data only.";
    case TileProvider::OpenStreetMap:
        return "Street map, no key. Not for offline use or heavy traffic, and may be "
               "blocked without notice under OpenStreetMap's tile policy.";
    case TileProvider::Google:
        return "A key is for the Map Tiles API, billed to your Google account.";
    case TileProvider::Sentinel2:
        return "Copernicus imagery from EOX, no key, softer than the others and with "
               "nothing past zoom 14.";
    case TileProvider::NasaGibs:
        return "NASA imagery, no key. Coarse: nothing past zoom 12, or zoom 9 for the daily "
               "infrared layers. A daily layer shows one day's passes, often with cloud or gaps, "
               "so step through dates to find a clear one.";
    case TileProvider::Custom:
        return "Any tile server: {z} {x} {y}, {s} for numbered servers 0 to 3, and {key} "
               "where it wants the key. "
               "You are responsible for following its terms.";
    }
    return "";
}

const char* tile_source_problem(const TileSourceConfig& c)
{
    switch (c.provider) {
    case TileProvider::Esri:
    case TileProvider::OpenStreetMap:
    case TileProvider::Google:
    case TileProvider::Sentinel2:
        return nullptr;
    case TileProvider::NasaGibs:
        if (tile_gibs_layer(c.gibs_layer).dated && !tile_date_valid(c.gibs_date))
            return "The date must be a real date, as YYYY-MM-DD";
        return nullptr;
    case TileProvider::Custom:
        if (c.custom_url.find("://") == std::string::npos ||
            c.custom_url.find("{z}") == std::string::npos ||
            c.custom_url.find("{x}") == std::string::npos ||
            c.custom_url.find("{y}") == std::string::npos)
            return "The URL needs http(s):// and {z} {x} {y}";
        if (c.custom_url.find("{key}") != std::string::npos && c.custom_key.empty())
            return "The URL has {key} but no key is set";
        return nullptr;
    }
    return "Unknown map source";
}

const char* tile_user_agent(const std::string& host)
{
    // Firefox's current ESR, which needs raising now and then to stay believable. macOS
    // stays at 10.15 because Firefox itself reports that on every later version.
#if defined(_WIN32)
    static constexpr char BROWSER[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:153.0) Gecko/20100101 Firefox/153.0";
#elif defined(__APPLE__)
    static constexpr char BROWSER[] = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:153.0) Gecko/20100101 Firefox/153.0";
#elif defined(__linux__)
    static constexpr char BROWSER[] = "Mozilla/5.0 (X11; Linux x86_64; rv:153.0) Gecko/20100101 Firefox/153.0";
#else
    static constexpr const char* BROWSER = GCS_USER_AGENT;
#endif

    // Just the name, as DNS would read it, so that no way of writing a custom URL for
    // OpenStreetMap gets past the check below.
    const size_t scheme = host.find("://");
    std::string name = scheme == std::string::npos ? host : host.substr(scheme + 3);
    name = name.substr(0, name.find(':'));
    if (!name.empty() && name.back() == '.') name.pop_back();
    for (char& ch : name) ch = (char)std::tolower((unsigned char)ch);

    static const std::string OSM_DOMAIN = ".openstreetmap.org";
    if (name == "openstreetmap.org" ||
        (name.size() > OSM_DOMAIN.size() &&
         name.compare(name.size() - OSM_DOMAIN.size(), std::string::npos, OSM_DOMAIN) == 0))
        return GCS_USER_AGENT;

    for (const char* named : { "tile.googleapis.com", "ibasemaps-api.arcgis.com",
                               "tiles.maps.eox.at", "gibs.earthdata.nasa.gov" })
        if (name == named) return GCS_USER_AGENT;
    return BROWSER;
}

std::string tile_url_encode(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (const unsigned char ch : s) {
        const bool plain = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                           (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~';
        if (plain) {
            out += (char)ch;
        } else {
            char esc[4];
            snprintf(esc, sizeof(esc), "%%%02X", ch);
            out += esc;
        }
    }
    return out;
}

std::string tile_source_template(const TileSourceConfig& c, const std::string& session)
{
    switch (c.provider) {
    case TileProvider::Esri:
        if (c.kind == TileKind::Terrain) return TILE_ESRI_TOPO_URL;
        if (c.esri_key.empty()) return TILE_ESRI_URL;
        return std::string(TILE_ESRI_KEYED_URL) + "?token=" + tile_url_encode(c.esri_key);
    case TileProvider::OpenStreetMap:
        return TILE_OSM_URL;
    case TileProvider::Google:
        if (c.google_key.empty()) {
            std::string url = TILE_GOOGLE_KEYLESS_URL;
            const size_t p = url.find("lyrs=s");
            url[p + 5] = c.kind == TileKind::Hybrid ? 'y' : c.kind == TileKind::Terrain ? 'p' : 's';
            return url;
        }
        return std::string(TILE_GOOGLE_HOST) + "/v1/2dtiles/{z}/{x}/{y}?session=" +
               tile_url_encode(session) + "&key=" + tile_url_encode(c.google_key);
    case TileProvider::Sentinel2:
        return c.kind == TileKind::Terrain ? TILE_SENTINEL_TERRAIN_URL : TILE_SENTINEL_URL;
    case TileProvider::NasaGibs: {
        const TileGibsLayer& l = tile_gibs_layer(c.gibs_layer);
        return std::string("https://gibs.earthdata.nasa.gov/wmts/epsg3857/best/") + l.id + "/default/" +
               (l.dated ? c.gibs_date : std::string("default")) + "/GoogleMapsCompatible_Level" +
               std::to_string(l.max_zoom) + "/{z}/{y}/{x}." + l.extension;
    }
    case TileProvider::Custom: {
        std::string url = c.custom_url;
        const std::string key = tile_url_encode(c.custom_key);
        for (size_t p; (p = url.find("{key}")) != std::string::npos;) url.replace(p, 5, key);
        return url;
    }
    }
    return {};
}

std::string tile_source_overlay_template(const TileSourceConfig& c)
{
    if (c.kind != TileKind::Hybrid) return {};
    if (c.provider == TileProvider::Esri)      return TILE_ESRI_PLACES_URL;
    if (c.provider == TileProvider::Sentinel2) return TILE_SENTINEL_STREETS_URL;
    return {};   // Google's hybrid is one layer; the others have no hybrid
}

std::string tile_source_overlay_cache_dir(const TileSourceConfig& c)
{
    return c.provider == TileProvider::Esri ? "esri-places" : "sentinel-streets";
}

bool tile_source_cache_dir(const TileSourceConfig& c, std::string& subdir)
{
    switch (c.provider) {
    case TileProvider::Esri:
        subdir = c.kind == TileKind::Terrain ? "esri-topo" : "";
        return true;
    case TileProvider::Sentinel2:
        subdir = c.kind == TileKind::Terrain ? "sentinel-terrain" : "sentinel-2024";
        return true;
    case TileProvider::NasaGibs: {
        const TileGibsLayer& l = tile_gibs_layer(c.gibs_layer);
        subdir = std::string("gibs/") + l.id + (l.dated ? "/" + c.gibs_date : std::string());
        return true;
    }
    case TileProvider::OpenStreetMap:
        subdir = "osm";
        return true;
    case TileProvider::Google:
        subdir = c.kind == TileKind::Hybrid  ? "google-hybrid"
               : c.kind == TileKind::Terrain ? "google-terrain" : "google";
        return c.google_key.empty();
    case TileProvider::Custom: {
        // By the template as typed, so the key is not in the path, and a changed
        // key keeps its tiles while a changed server does not. FNV-1a: this only
        // has to tell servers apart.
        uint64_t h = 1469598103934665603ull;
        for (const unsigned char ch : c.custom_url) { h ^= ch; h *= 1099511628211ull; }
        char name[32];
        snprintf(name, sizeof(name), "custom-%016llx", (unsigned long long)h);
        subdir = name;
        return true;
    }
    }
    return false;
}

std::string tile_source_attribution(const TileSourceConfig& c)
{
    switch (c.provider) {
    case TileProvider::Esri:
        if (c.kind == TileKind::Terrain)
            return "Sources: Esri, HERE, Garmin, Intermap, USGS, FAO, NPS, \xc2\xa9 OpenStreetMap contributors, "
                   "and the GIS User Community";
        if (c.kind == TileKind::Hybrid)
            return std::string(TILE_ESRI_ATTRIBUTION) + " \xc2\xb7 Esri, HERE, Garmin, \xc2\xa9 OpenStreetMap contributors";
        return TILE_ESRI_ATTRIBUTION;
    case TileProvider::OpenStreetMap: return TILE_OSM_ATTRIBUTION;
    case TileProvider::NasaGibs: {
        const TileGibsLayer& l = tile_gibs_layer(c.gibs_layer);
        return std::string(l.credit) + (l.dated ? " " + c.gibs_date : std::string()) +
               ", via NASA GIBS, part of NASA ESDIS";
    }
    case TileProvider::Sentinel2: {
        const std::string s2 = "EOxCloudless https://cloudless.eox.at by EOX IT Services GmbH "
                               "(Contains modified Copernicus Sentinel data 2024)";
        if (c.kind == TileKind::Terrain)
            return "Terrain: Data \xc2\xa9 OpenStreetMap contributors and others, Rendering \xc2\xa9 EOX";
        if (c.kind == TileKind::Hybrid)
            return s2 + " \xc2\xb7 Overlay: Data \xc2\xa9 OpenStreetMap contributors, Rendering \xc2\xa9 EOX and MapServer";
        return s2;
    }
    case TileProvider::Google:        return TILE_GOOGLE_NAME;
    case TileProvider::Custom:        return c.custom_attribution;
    }
    return {};
}

std::string tile_google_session_path(const std::string& key)
{
    return "/v1/createSession?key=" + tile_url_encode(key);
}

std::string tile_google_session_body(TileKind kind)
{
    switch (kind) {
    case TileKind::Satellite:
        return R"({"mapType":"satellite","language":"en-US","region":"US"})";
    case TileKind::Hybrid:   // Google's documented equivalent of its hybrid map type
        return R"({"mapType":"satellite","language":"en-US","region":"US","layerTypes":["layerRoadmap"],"overlay":false})";
    case TileKind::Terrain:  // terrain requires the roadmap layer
        return R"({"mapType":"terrain","language":"en-US","region":"US","layerTypes":["layerRoadmap"]})";
    }
    return {};
}

bool tile_google_parse_session(const std::string& body, std::string& session, int64_t& expiry)
{
    try {
        const json j = json::parse(body);
        if (!j.is_object() || !j.contains("session") || !j["session"].is_string()) return false;
        session = j["session"].get<std::string>();
        if (session.empty()) return false;

        // Documented as seconds since the epoch, and sent as a string.
        expiry = 0;
        if (j.contains("expiry")) {
            if (j["expiry"].is_string())      expiry = std::stoll(j["expiry"].get<std::string>());
            else if (j["expiry"].is_number()) expiry = j["expiry"].get<int64_t>();
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string tile_google_viewport_path(const std::string& session, const std::string& key, int zoom,
                                      double north, double south, double east, double west)
{
    char bounds[160];
    snprintf(bounds, sizeof(bounds), "&zoom=%d&north=%.6f&south=%.6f&east=%.6f&west=%.6f",
             zoom, north, south, east, west);
    return "/tile/v1/viewport?session=" + tile_url_encode(session) + "&key=" + tile_url_encode(key) + bounds;
}

std::string tile_google_parse_copyright(const std::string& body)
{
    try {
        const json j = json::parse(body);
        if (j.is_object() && j.contains("copyright") && j["copyright"].is_string())
            return j["copyright"].get<std::string>();
    } catch (const std::exception&) {
    }
    return {};
}
