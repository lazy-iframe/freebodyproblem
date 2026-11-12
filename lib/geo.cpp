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

// lib/geo.cpp

#include "lib/geo.hpp"

#include <algorithm>
#include <cmath>

double geo_distance_m(double lat1, double lon1, double lat2, double lon2)
{
    constexpr double A = 6378137.0, F = 1.0 / 298.257223563, B = A * (1.0 - F);
    constexpr double D2R = GEO_DEG_TO_RAD;

    const double L  = (lon2 - lon1) * D2R;
    const double U1 = std::atan((1.0 - F) * std::tan(lat1 * D2R));
    const double U2 = std::atan((1.0 - F) * std::tan(lat2 * D2R));
    const double sU1 = std::sin(U1), cU1 = std::cos(U1), sU2 = std::sin(U2), cU2 = std::cos(U2);

    double lam = L;
    for (int i = 0; i < 100; ++i) {
        const double sl = std::sin(lam), cl = std::cos(lam);
        const double ss = std::sqrt((cU2 * sl) * (cU2 * sl) +
                                    (cU1 * sU2 - sU1 * cU2 * cl) * (cU1 * sU2 - sU1 * cU2 * cl));
        if (ss == 0.0) return 0.0;   // the same point
        const double cs  = sU1 * sU2 + cU1 * cU2 * cl;
        const double sig = std::atan2(ss, cs);
        const double sa  = cU1 * cU2 * sl / ss;
        const double c2a = 1.0 - sa * sa;
        const double c2m = c2a != 0.0 ? cs - 2.0 * sU1 * sU2 / c2a : 0.0;   // 0 along the equator
        const double C   = F / 16.0 * c2a * (4.0 + F * (4.0 - 3.0 * c2a));
        const double next = L + (1.0 - C) * F * sa *
                            (sig + C * ss * (c2m + C * cs * (-1.0 + 2.0 * c2m * c2m)));
        if (std::fabs(next - lam) < 1e-12) {
            const double u2 = c2a * (A * A - B * B) / (B * B);
            const double k1 = 1.0 + u2 / 16384.0 * (4096.0 + u2 * (-768.0 + u2 * (320.0 - 175.0 * u2)));
            const double k2 = u2 / 1024.0 * (256.0 + u2 * (-128.0 + u2 * (74.0 - 47.0 * u2)));
            const double ds = k2 * ss * (c2m + k2 / 4.0 * (cs * (-1.0 + 2.0 * c2m * c2m) -
                              k2 / 6.0 * c2m * (-3.0 + 4.0 * ss * ss) * (-3.0 + 4.0 * c2m * c2m)));
            return B * k1 * (sig - ds);
        }
        lam = next;
    }

    const double dlat = (lat2 - lat1) * D2R, dlon = (lon2 - lon1) * D2R;
    const double h = std::sin(dlat / 2) * std::sin(dlat / 2) +
                     std::cos(lat1 * D2R) * std::cos(lat2 * D2R) * std::sin(dlon / 2) * std::sin(dlon / 2);
    return 2.0 * 6371008.8 * std::asin(std::sqrt(h < 1.0 ? h : 1.0));
}

double geo_rough_distance_m(double lat1, double lon1, double lat2, double lon2)
{
    const double dn = (lat2 - lat1) * GEO_M_PER_DEG;
    const double de = (lon2 - lon1) * GEO_M_PER_DEG *
                      std::cos(0.5 * (lat1 + lat2) * GEO_DEG_TO_RAD);
    return std::sqrt(dn * dn + de * de);
}

void geo_offset(double lat, double lon, double north_m, double east_m,
                double& out_lat, double& out_lon)
{
    out_lat = lat + north_m / GEO_M_PER_DEG;
    out_lon = lon + east_m / (GEO_M_PER_DEG * std::cos(lat * GEO_DEG_TO_RAD));
    if (out_lon >  180.0) out_lon -= 360.0;
    if (out_lon < -180.0) out_lon += 360.0;
}

void geo_mercator(double lat, double lon, double& u, double& v)
{
    lat = std::clamp(lat, -GEO_MERCATOR_MAX_LAT, GEO_MERCATOR_MAX_LAT);
    const double r = lat * GEO_DEG_TO_RAD;
    u = (lon + 180.0) / 360.0;
    v = (1.0 - std::log(std::tan(r) + 1.0 / std::cos(r)) / GEO_PI) / 2.0;
}

void geo_unmercator(double u, double v, double& lat, double& lon)
{
    lon = u * 360.0 - 180.0;
    lat = std::atan(std::sinh(GEO_PI * (1.0 - 2.0 * v))) / GEO_DEG_TO_RAD;
}

double geo_mercator_world_m(double lat)
{
    constexpr double EQUATOR_M = 40075016.686;   // 2π × WGS84's equatorial radius
    return EQUATOR_M * std::cos(lat * GEO_DEG_TO_RAD);
}

void geo_to_tile(double lat, double lon, int z, double& tx, double& ty)
{
    const double n = (double)(1 << z);
    geo_mercator(lat, lon, tx, ty);
    tx *= n;
    ty *= n;
}

void geo_from_tile(double tx, double ty, int z, double& lat, double& lon)
{
    const double n = (double)(1 << z);
    geo_unmercator(tx / n, ty / n, lat, lon);
}
