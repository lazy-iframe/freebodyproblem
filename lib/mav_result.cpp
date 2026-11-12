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

// lib/mav_result.cpp

#include "lib/mav_result.hpp"

#include <mavlink/ardupilotmega/mavlink.h>

const char* mav_result_name(uint8_t r)
{
    switch (r) {
    case MAV_RESULT_ACCEPTED:                       return "ACCEPTED";
    case MAV_RESULT_TEMPORARILY_REJECTED:           return "TEMPORARILY_REJECTED";
    case MAV_RESULT_DENIED:                         return "DENIED";
    case MAV_RESULT_UNSUPPORTED:                    return "UNSUPPORTED";
    case MAV_RESULT_FAILED:                         return "FAILED";
    case MAV_RESULT_IN_PROGRESS:                    return "IN_PROGRESS";
    case MAV_RESULT_CANCELLED:                      return "CANCELLED";
    case MAV_RESULT_COMMAND_LONG_ONLY:              return "COMMAND_LONG_ONLY";
    case MAV_RESULT_COMMAND_INT_ONLY:               return "COMMAND_INT_ONLY";
    case MAV_RESULT_COMMAND_UNSUPPORTED_MAV_FRAME:  return "COMMAND_UNSUPPORTED_MAV_FRAME";
    default:                                        return "?";
    }
}

const char* mav_result_text(uint8_t r)
{
    switch (r) {
    case MAV_RESULT_ACCEPTED:                       return "accepted";
    case MAV_RESULT_TEMPORARILY_REJECTED:           return "temporarily rejected";
    case MAV_RESULT_DENIED:                         return "denied";
    case MAV_RESULT_UNSUPPORTED:                    return "unsupported";
    case MAV_RESULT_FAILED:                         return "failed";
    case MAV_RESULT_IN_PROGRESS:                    return "in progress";
    case MAV_RESULT_CANCELLED:                      return "cancelled";
    case MAV_RESULT_COMMAND_LONG_ONLY:              return "only accepted as COMMAND_LONG";
    case MAV_RESULT_COMMAND_INT_ONLY:               return "only accepted as COMMAND_INT";
    case MAV_RESULT_COMMAND_UNSUPPORTED_MAV_FRAME:  return "unsupported frame";
    default:                                        return "rejected";
    }
}
