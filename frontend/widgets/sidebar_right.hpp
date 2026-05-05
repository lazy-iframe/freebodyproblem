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
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "../../backend/mavlink_parser.hpp"
#include "../../backend/mavlink_sender.hpp"
#include "../settings.hpp"

void draw_sidebar_right(const VehicleState& vs,
                        const std::vector<StatusText>& status_texts,
                        MavlinkSender* sender,
                        AppSettings* settings);
