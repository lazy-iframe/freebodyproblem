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

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#else
#  include <netinet/in.h>
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

#include "connection.hpp"
#include "mavlink_sender.hpp"

// Where to point a link and how to talk to it.
struct LinkConfig {
    ConnType type       = ConnType::UDP;
    char     host[64]   = "0.0.0.0";   // UDP bind host / TCP remote host
    int      port       = 14550;
    char     device[64] = {};          // serial device path
    int      baud       = 57600;
};

// What a receive attempt actually meant.
//
// A read that returned nothing is not a read that failed, and telling them
// apart is what distinguishes a quiet link from a dead one. Every non-positive
// result used to be treated as "nothing yet", so a serial port whose device had
// disappeared — an autopilot re-enumerating its USB after a reboot, a radio
// unplugged — and a TCP peer that had hung up both looked exactly like a
// vehicle with nothing to say. The thread then span on a dead handle forever
// while the UI went on reporting a healthy link.
enum class RxResult { Data, Quiet, Closed, Failed };

// Process-wide socket setup. No-ops off Windows; safe to call repeatedly.
void link_sockets_init();
void link_sockets_cleanup();

// A link as the UI sees it: enough to list it and to name it for disconnection,
// with none of the machinery behind it. Lives here rather than with the fleet so
// that a panel drawing the list does not have to include the threading.
struct LinkInfo {
    uint32_t    id       = 0;
    std::string name;                        // "udp:0.0.0.0:14550"
    LinkStatus  status   = LinkStatus::Idle;
    int         vehicles = 0;                // heard on this link
};

// One transport: a UDP socket, a TCP connection, or a serial port.
//
// Everything platform-specific about opening and reading lives behind this, so
// the thread that drives it deals only in bytes and an RxResult. The link owns
// the handle, which is why sending also goes through here: a vehicle queues
// frames in its own sender, and the link drains that queue into the transport
// it alone knows how to write to.
class Link {
public:
    Link(uint32_t id, const LinkConfig& cfg);
    ~Link();

    Link(const Link&)            = delete;
    Link& operator=(const Link&) = delete;

    // Opens the transport. On failure the status says which kind, and the
    // caller should not read.
    bool open();
    void close();

    // Blocks for up to ~200 ms. `n` receives the byte count, `src` the datagram
    // source where the transport has one.
    RxResult read(uint8_t* buf, size_t cap, int& n, sockaddr_in& src, bool& src_valid);

    // Drains one vehicle's outbound queue into this transport. `peer` is used
    // only by UDP, which is the one transport where a single handle can be
    // pointed at several vehicles.
    void flush_to(MavlinkSender& sender, const sockaddr_in* peer);

    LinkStatus  status() const { return status_.load(std::memory_order_relaxed); }
    void        set_status(LinkStatus s) { status_.store(s, std::memory_order_relaxed); }
    uint32_t    id()     const { return id_; }

    // "udp:0.0.0.0:14550", "serial:/dev/ttyACM0@57600" — for the UI.
    const std::string& name() const { return name_; }

private:
    const uint32_t   id_;
    const LinkConfig cfg_;
    std::string      name_;

    int          fd_        = -1;
    SerialHandle ser_       = SERIAL_INVALID;
    bool         is_stream_ = false;
    bool         is_serial_ = false;

    std::atomic<LinkStatus> status_{LinkStatus::Idle};
};
