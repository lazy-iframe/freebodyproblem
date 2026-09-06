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

#include <functional>
#include <unordered_map>

#include "../../backend/vehicle.hpp"

// Panel state that belongs to a vehicle rather than to a panel.
//
// A file-scope static in a tab was the right thing to be when there was one
// vehicle: the RC channel set, the staged parameter edits, the mission being
// planned, the set of messages already asked for. With a fleet on screen every
// one of them is a per-vehicle fact wearing a global's clothes, and the failure
// is quiet — switching the callsign chip leaves one aircraft's mission in the
// editor pointed at another, or tells the panel it has already requested a
// message stream from a vehicle it has never spoken to.
//
// Wrapping such a static in VehicleUiState keys it by vehicle. Reads go to
// whichever vehicle is currently bound, so the call sites barely change:
//
//     static VehicleUiState<RcTabState> s_state;
//     static RcTabState& st() { return *s_state; }   // then use st().cal
//
// Keying rather than clearing on switch, because most of this is work in
// progress. A compass calibration is a conversation the vehicle is driving and
// a half-planned mission is the operator's work; glancing at another aircraft
// must not end either. Everything here is touched only on the UI thread, so
// none of it locks.

// The vehicle whose state the panels are reading and writing. Set once per
// panel per frame by the render loop.
void      ui_bind_vehicle(VehicleId id);
VehicleId ui_bound_vehicle();

// Drop everything held for a vehicle that has gone, across every
// VehicleUiState in the program. Called by the render loop when a vehicle
// disappears, so a reconnected link does not inherit the last airframe's
// half-run calibration or its staged edits.
void ui_forget_vehicle(VehicleId id);

namespace detail {
// Each VehicleUiState registers an eraser here so ui_forget_vehicle() can reach
// state it has no other way of knowing about.
void ui_register_forgetter(std::function<void(VehicleId)> fn);
} // namespace detail

template <class T>
class VehicleUiState {
public:
    VehicleUiState()
    {
        detail::ui_register_forgetter([this](VehicleId id) { map_.erase(id); });
    }

    VehicleUiState(const VehicleUiState&)            = delete;
    VehicleUiState& operator=(const VehicleUiState&) = delete;

    T& operator*()  { return map_[ui_bound_vehicle()]; }
    T* operator->() { return &map_[ui_bound_vehicle()]; }

private:
    std::unordered_map<VehicleId, T, VehicleIdHash> map_;
};
