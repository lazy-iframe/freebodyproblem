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


#include "vehicle_ui_state.hpp"

#include <vector>

namespace {

VehicleId& bound()
{
    static VehicleId id{};
    return id;
}

// Function-local rather than a file-scope vector: the VehicleUiState instances
// registering here are themselves file-scope statics in other translation
// units, and their construction order against a plain global is unspecified.
std::vector<std::function<void(VehicleId)>>& forgetters()
{
    static std::vector<std::function<void(VehicleId)>> v;
    return v;
}

} // namespace

void ui_bind_vehicle(VehicleId id) { bound() = id; }

VehicleId ui_bound_vehicle() { return bound(); }

void ui_forget_vehicle(VehicleId id)
{
    for (auto& fn : forgetters()) fn(id);
}

namespace detail {

void ui_register_forgetter(std::function<void(VehicleId)> fn)
{
    forgetters().push_back(std::move(fn));
}

} // namespace detail
