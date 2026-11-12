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

// lib/geo.hpp
//
// Positions on the earth: how far apart two are, where a point lands moved
// some metres north and east, and Web Mercator, the projection the map tiles
// and the terrain tiles are cut in. Degrees in, metres out, throughout.
//
// Two distances, for two kinds of question. geo_distance_m() is exact, for a
// figure the operator reads. geo_rough_distance_m() and geo_offset() treat
// the earth as flat about the point: off by well under a metre over the few
// kilometres they are asked about, and cheap enough to run per sample, per
// frame.

#pragma once

inline constexpr double GEO_PI = 3.14159265358979323846;
inline constexpr double GEO_DEG_TO_RAD = GEO_PI / 180.0;

// Metres in a degree of latitude, and of longitude at the equator (WGS84's
// equatorial radius times π/180, rounded).
inline constexpr double GEO_M_PER_DEG = 111320.0;

// Web Mercator's limit: the latitude at which the square world ends.
inline constexpr double GEO_MERCATOR_MAX_LAT = 85.0511;

// The shortest distance over the WGS84 ellipsoid, at sea level. Vincenty's
// inverse formula; for points all but opposite on the earth, where it fails to
// settle, a sphere, off by under a percent at a range no vehicle flies.
double geo_distance_m(double lat1, double lon1, double lat2, double lon2);

// Flat-earth distance about the two points' mean latitude.
double geo_rough_distance_m(double lat1, double lon1, double lat2, double lon2);

// The point `north_m` north and `east_m` east of (lat, lon), flat-earth.
// Longitude is wrapped into -180..180.
void geo_offset(double lat, double lon, double north_m, double east_m,
                double& out_lat, double& out_lon);

// Web Mercator as a fraction of the world: u 0..1 east from the antimeridian,
// v 0..1 south from the top. Latitude is held to GEO_MERCATOR_MAX_LAT.
void geo_mercator(double lat, double lon, double& u, double& v);
void geo_unmercator(double u, double v, double& lat, double& lon);

// The same in tiles at zoom `z`: the world is 2^z tiles across, and the
// integer part names the tile.
void geo_to_tile(double lat, double lon, int z, double& tx, double& ty);
void geo_from_tile(double tx, double ty, int z, double& lat, double& lon);

// Ground metres the whole Mercator world spans at a latitude: the scale that
// turns a u/v step into metres there, east and north alike.
double geo_mercator_world_m(double lat);
