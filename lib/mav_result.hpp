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

// lib/mav_result.hpp
//
// MAV_RESULT, the verdict a COMMAND_ACK carries, as words. Two forms: the
// enum's own name for protocol traces, where it is matched against the spec,
// and plain words for what the operator reads.

#pragma once

#include <cstdint>

// "ACCEPTED", "TEMPORARILY_REJECTED", …; "?" for a value the spec does not have.
const char* mav_result_name(uint8_t result);

// "accepted", "temporarily rejected", …; "rejected" for a value the spec does
// not have, since anything but ACCEPTED and IN_PROGRESS is a refusal.
const char* mav_result_text(uint8_t result);
