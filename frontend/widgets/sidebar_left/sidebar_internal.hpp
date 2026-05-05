// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer
// Contact: azhar.tanweer404@gmail.com

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#pragma once
// Internal declarations shared between sidebar_left.cpp (orchestrator)
// and the per-tab translation units. Not part of the public API.
#include "../sidebar_left.hpp"  // ConnectionRequest, LinkStatus, VehicleState, ParamEntry, AppSettings
#include <unordered_map>
#include <string>
#include <cstdint>

void draw_tab_connection(const VehicleState* vs,
                         ConnectionRequest* conn_out,
                         LinkStatus link_status,
                         AppSettings* settings);

void draw_tab_flight(MavlinkSender* sender, const VehicleState* vs);

void draw_tab_params(MavlinkSender* sender, const VehicleState* vs,
                     const std::unordered_map<std::string, ParamEntry>* params);

void draw_tab_themes(AppSettings* settings);

void draw_tab_mission(MavlinkSender* sender, const VehicleState* vs,
                      MissionPickState* pick);

void draw_tab_mavlink(MavlinkSender* sender,
                      const VehicleState* vs,
                      const std::unordered_map<uint32_t, MessageStats>* stats,
                      uint64_t total_messages,
                      uint64_t total_bytes,
                      uint64_t parse_errors);
