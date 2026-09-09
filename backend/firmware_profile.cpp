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


#include "firmware_profile.hpp"
#include "mavlink_sender.hpp"

#include <cctype>
#include <mavlink/ardupilotmega/mavlink.h>

namespace {

// ── ArduPilot ─────────────────────────────────────────────────────────────────

// ArduCopter's flat mode numbering. Frame-specific: a Plane's 3 is TRAINING
// where a Copter's is AUTO, which is why this is only ever the fallback for a
// vehicle that will not publish AVAILABLE_MODES.
namespace copter {
    constexpr uint32_t STABILIZE = 0;
    constexpr uint32_t ACRO      = 1;
    constexpr uint32_t ALT_HOLD  = 2;
    constexpr uint32_t AUTO      = 3;
    constexpr uint32_t GUIDED    = 4;
    constexpr uint32_t LOITER    = 5;
    constexpr uint32_t RTL       = 6;
    constexpr uint32_t LAND      = 9;
}

class ArduPilotProfile final : public FirmwareProfile {
public:
    const char* name()             const override { return "ArduPilot"; }
    uint32_t    estimator_msgid()  const override { return MAVLINK_MSG_ID_EKF_STATUS_REPORT; }
    const char* estimator_units()  const override { return "VARIANCE"; }

    const std::vector<FallbackMode>& fallback_modes(MAV_TYPE) const override
    {
        // RTL is in this list rather than on a button of its own. A vehicle
        // that publishes AVAILABLE_MODES already offers it, and a second
        // control for the same mode is one the operator has to think about.
        // It has to be here, though: without it a stack that never answers
        // would have no way to reach RTL at all.
        static const std::vector<FallbackMode> k_modes = {
            { "STAB", copter::STABILIZE },
            { "ACRO", copter::ACRO      },
            { "ALTH", copter::ALT_HOLD  },
            { "LOIT", copter::LOITER    },
            { "GUID", copter::GUIDED    },
            { "AUTO", copter::AUTO      },
            { "RTL",  copter::RTL       },
            { "LAND", copter::LAND      },
        };
        return k_modes;
    }

    std::string mode_short_name(uint32_t custom_mode, MAV_TYPE) const override
    {
        switch (custom_mode) {
        case 0:  return "STAB";
        case 1:  return "ACRO";
        case 2:  return "ALTH";
        case 3:  return "AUTO";
        case 4:  return "GUID";
        case 5:  return "LOIT";
        case 6:  return "RTL";
        case 7:  return "CIRC";
        case 9:  return "LAND";
        case 11: return "DRFT";
        case 13: return "SPRT";
        case 16: return "POSH";
        case 17: return "BRAKE";
        case 20: return "GUID-NG";
        case 21: return "SMRTRTL";
        default: return "MODE ?";
        }
    }

    SetModeCommand encode_set_mode(uint32_t custom_mode, uint8_t) const override
    {
        // The whole flat number in param2, param3 unused — what this GCS has
        // always sent, and what ArduPilot reads.
        //
        // standard_mode is ignored on purpose. ArduPilot may well advertise
        // standard modes, but DO_SET_STANDARD_MODE has not been tried against
        // it here, and this path is the one that is known to work. Switching
        // it over wants an ArduPilot to test against first.
        return { 176, (float)MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                 (float)custom_mode, 0.0f };
    }

    Capability position_target(const VehicleState& vs) const override
    {
        // The vehicle's own AVAILABLE_MODES list is the authority — custom_mode
        // numbering is per-frame, so the table below is only correct by
        // accident on anything that publishes one.
        for (const FlightModeInfo& m : vs.available_modes) {
            if (m.custom_mode != vs.custom_mode) continue;
            std::string n;
            n.reserve(m.name.size());
            for (char c : m.name)
                n.push_back((char)std::tolower((unsigned char)c));
            // "Guided" and "Guided NoGPS" are two different modes; the latter
            // takes attitude targets rather than positions.
            return n == "guided" ? ok() : refused();
        }

        switch (vs.type) {
        case MAV_TYPE_FIXED_WING:
        case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
        case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_FIXEDROTOR:
        case MAV_TYPE_VTOL_TAILSITTER:
        case MAV_TYPE_VTOL_TILTWING:
        case MAV_TYPE_GROUND_ROVER:
        case MAV_TYPE_SURFACE_BOAT:
            return vs.custom_mode == 15 ? ok() : refused();   // Plane / Rover GUIDED
        default:
            return vs.custom_mode == 4 ? ok() : refused();    // Copter / Sub GUIDED
        }
    }

    GotoMethod goto_method() const override
    { return GotoMethod::PositionTarget; }   // honoured in GUIDED

    Capability aux_functions() const override { return { true, nullptr }; }

private:
    static Capability ok()      { return { true,  nullptr }; }
    static Capability refused() { return { false, "GUIDED MODE REQUIRED" }; }
};

// ── PX4 ───────────────────────────────────────────────────────────────────────

class Px4Profile final : public FirmwareProfile {
public:
    const char* name()             const override { return "PX4"; }
    uint32_t    estimator_msgid()  const override { return MAVLINK_MSG_ID_ESTIMATOR_STATUS; }
    const char* estimator_units()  const override { return "INNOV. RATIO"; }

    const std::vector<FallbackMode>& fallback_modes(MAV_TYPE) const override
    {
        // No frame-type argument used, unlike ArduPilot's: PX4 numbers its
        // modes the same way whatever the airframe.
        //
        // Written from PX4's documented mode enums rather than observed off a
        // vehicle — there is no PX4 here to read them from. Only reached on
        // PX4 older than v1.15; anything newer publishes AVAILABLE_MODES and
        // never gets this far.
        static const std::vector<FallbackMode> k_modes = {
            { "MANU", px4_mode::pack(px4_mode::MANUAL)                      },
            { "STAB", px4_mode::pack(px4_mode::STABILIZED)                  },
            { "ACRO", px4_mode::pack(px4_mode::ACRO)                        },
            { "ALTC", px4_mode::pack(px4_mode::ALTCTL)                      },
            { "POSC", px4_mode::pack(px4_mode::POSCTL)                      },
            { "HOLD", px4_mode::pack(px4_mode::AUTO, px4_mode::LOITER)      },
            { "MISN", px4_mode::pack(px4_mode::AUTO, px4_mode::MISSION)     },
            { "TKOF", px4_mode::pack(px4_mode::AUTO, px4_mode::TAKEOFF)     },
            { "LAND", px4_mode::pack(px4_mode::AUTO, px4_mode::LAND)        },
            { "RTL",  px4_mode::pack(px4_mode::AUTO, px4_mode::RTL)         },
            { "OFFB", px4_mode::pack(px4_mode::OFFBOARD)                    },
        };
        return k_modes;
    }

    std::string mode_short_name(uint32_t custom_mode, MAV_TYPE) const override
    {
        switch (px4_mode::main_of(custom_mode)) {
        case px4_mode::MANUAL:     return "MANU";
        case px4_mode::ALTCTL:     return "ALTC";
        case px4_mode::POSCTL:     return "POSC";
        case px4_mode::ACRO:       return "ACRO";
        case px4_mode::OFFBOARD:   return "OFFB";
        case px4_mode::STABILIZED: return "STAB";
        case px4_mode::AUTO:
            switch (px4_mode::sub_of(custom_mode)) {
            case px4_mode::READY:         return "READY";
            case px4_mode::TAKEOFF:       return "TKOF";
            case px4_mode::LOITER:        return "HOLD";
            case px4_mode::MISSION:       return "MISN";
            case px4_mode::RTL:           return "RTL";
            case px4_mode::LAND:          return "LAND";
            case px4_mode::FOLLOW_TARGET: return "FOLW";
            case px4_mode::PRECLAND:      return "PRECL";
            default:                      return "AUTO";
            }
        default: return "MODE ?";
        }
    }

    SetModeCommand encode_set_mode(uint32_t custom_mode,
                                   uint8_t standard_mode) const override
    {
        // A mode PX4 calls standard is set by its standard number, which is
        // both what the protocol says and the only thing that works. Orbit is
        // the case that proves it: PX4 advertises it as POSCTL sub-mode 1, but
        // asking for that sub-mode through DO_SET_MODE is *accepted* and lands
        // in Position Hold — a wrong mode with a cheerful ACK. Through
        // DO_SET_STANDARD_MODE it goes where it says.
        if (standard_mode != 0)
            return { 262, (float)standard_mode, 0.0f, 0.0f };

        // Otherwise main and sub split across param2 and param3. Emphatically
        // not the packed word — see the note on SetModeCommand. Both values are
        // at most 255, so a float carries them exactly.
        return { 176, (float)MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                 (float)px4_mode::main_of(custom_mode),
                 (float)px4_mode::sub_of(custom_mode) };
    }

    Capability position_target(const VehicleState&) const override
    {
        // Ungated, and it can afford to be. PX4 goes by way of
        // MAV_CMD_DO_REPOSITION (see goto_method), which is a command: a mode
        // that will not take it answers with a rejection the operator can see,
        // so there is nothing for this table to second-guess. Contrast the
        // setpoint message ArduPilot uses, which is silent either way and so
        // has to be gated on the mode up front.
        return { true, nullptr };
    }

    GotoMethod goto_method() const override
    { return GotoMethod::Reposition; }

    Capability aux_functions() const override
    {
        return { false, "AUX FUNCTIONS ARE ARDUPILOT-ONLY (MAV_CMD_DO_AUX_FUNCTION)" };
    }
};

} // namespace

const FirmwareProfile& firmware_profile(MAV_AUTOPILOT autopilot)
{
    // Immortal, const and stateless, which is what makes a bare reference safe
    // to hand to the UI thread and to every vehicle's protocol thread at once.
    // Function-local statics so construction order against other translation
    // units cannot matter.
    static const ArduPilotProfile s_ardupilot;
    static const Px4Profile       s_px4;

    if (autopilot == MAV_AUTOPILOT_PX4)
        return s_px4;
    return s_ardupilot;
}

void send_goto(MavlinkSender& sender, uint8_t tsys, uint8_t tcomp,
               const FirmwareProfile& profile,
               double lat, double lon, float altitude_m)
{
    switch (profile.goto_method()) {
    case FirmwareProfile::GotoMethod::Reposition:
        sender.reposition(tsys, tcomp, lat, lon, altitude_m);
        break;
    case FirmwareProfile::GotoMethod::PositionTarget:
    default:
        sender.goto_position(tsys, tcomp, lat, lon, altitude_m);
        break;
    }
}

void send_set_mode(MavlinkSender& sender, uint8_t tsys, uint8_t tcomp,
                   const FirmwareProfile& profile, uint32_t custom_mode,
                   uint8_t standard_mode)
{
    const SetModeCommand c = profile.encode_set_mode(custom_mode, standard_mode);
    if (c.command == 176)
        sender.set_mode_raw(tsys, tcomp, c.param1, c.param2, c.param3);
    else
        sender.command_long(tsys, tcomp, c.command, c.param1, c.param2, c.param3);
}
