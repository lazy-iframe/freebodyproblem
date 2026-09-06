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


#include "link.hpp"

#ifdef _WIN32
#  include <ws2tcpip.h>
   typedef int ssize_t;
#  define close_socket ::closesocket
#else
#  include <arpa/inet.h>
#  include <sys/socket.h>
#  include <sys/select.h>
#  include <sys/time.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <cerrno>
#  include <cstring>
#  define close_socket ::close
#endif

#include <cstdio>

#include "../frontend/app_log.hpp"   // gcs_log — thread-safe, see the header

namespace {

constexpr double CONNECT_TIMEOUT_S = 10.0;

#ifdef _WIN32
inline int  last_net_error() { return WSAGetLastError(); }
inline bool net_error_is_transient(int e)
{
    return e == WSAEWOULDBLOCK || e == WSAETIMEDOUT || e == WSAEINTR;
}
#else
inline int  last_net_error() { return errno; }
inline bool net_error_is_transient(int e)
{
    return e == EAGAIN || e == EWOULDBLOCK || e == EINTR || e == ETIMEDOUT;
}
#endif

} // namespace


void link_sockets_init()
{
#ifdef _WIN32
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
#endif
}

void link_sockets_cleanup()
{
#ifdef _WIN32
    WSACleanup();
#endif
}


Link::Link(uint32_t id, const LinkConfig& cfg)
    : id_(id), cfg_(cfg)
{
    char buf[160];
    switch (cfg_.type) {
    case ConnType::UDP:
        std::snprintf(buf, sizeof(buf), "udp:%s:%d", cfg_.host, cfg_.port);
        break;
    case ConnType::TCP:
        std::snprintf(buf, sizeof(buf), "tcp:%s:%d", cfg_.host, cfg_.port);
        break;
    case ConnType::Serial:
        std::snprintf(buf, sizeof(buf), "serial:%s@%d", cfg_.device, cfg_.baud);
        break;
    }
    name_ = buf;
}

Link::~Link() { close(); }

bool Link::open()
{
    link_sockets_init();
    status_.store(LinkStatus::Connecting, std::memory_order_relaxed);

    is_serial_ = (cfg_.type == ConnType::Serial);

    if (cfg_.type == ConnType::UDP) {
        fd_ = (int)socket(PF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) {
            gcs_log("%s: socket() failed", name_.c_str());
            status_.store(LinkStatus::Error);
            return false;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, cfg_.host, &addr.sin_addr);
        addr.sin_port = htons((uint16_t)cfg_.port);
        if (bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            gcs_log("%s: bind() failed", name_.c_str());
            close_socket(fd_);
            fd_ = -1;
            status_.store(LinkStatus::Error);
            return false;
        }
        // Short poll interval so the thread can check for shutdown and timeouts.
#ifdef _WIN32
        DWORD tv_ms = 200;
        setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_ms, sizeof(tv_ms));
        DWORD snd_ms = 500;
        setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&snd_ms, sizeof(snd_ms));
#else
        timeval tv{0, 200'000};
        setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        // A send that cannot complete must fail rather than park the link
        // thread; see the note in serial_write().
        timeval stv{0, 500'000};
        setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &stv, sizeof(stv));
#endif
        is_stream_ = false;
        return true;
    }

    if (cfg_.type == ConnType::TCP) {
#ifndef _WIN32
        fd_ = (int)socket(PF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            gcs_log("%s: socket() failed", name_.c_str());
            status_.store(LinkStatus::Error);
            return false;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, cfg_.host, &addr.sin_addr);
        addr.sin_port = htons((uint16_t)cfg_.port);

        // Non-blocking connect with a 10 s timeout.
        int flags = fcntl(fd_, F_GETFL, 0);
        fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
        int rc = connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (rc == -1 && errno == EINPROGRESS) {
            fd_set wfds, efds;
            FD_ZERO(&wfds); FD_SET(fd_, &wfds);
            FD_ZERO(&efds); FD_SET(fd_, &efds);
            timeval tv{(long)CONNECT_TIMEOUT_S, 0};
            rc = select(fd_ + 1, nullptr, &wfds, &efds, &tv);
            if (rc == 0) {
                gcs_log("%s: connect timeout", name_.c_str());
                close_socket(fd_);
                fd_ = -1;
                status_.store(LinkStatus::Timeout);
                return false;
            }
            if (rc < 0 || FD_ISSET(fd_, &efds)) {
                gcs_log("%s: connect failed", name_.c_str());
                close_socket(fd_);
                fd_ = -1;
                status_.store(LinkStatus::Error);
                return false;
            }
            // Verify the connection actually succeeded.
            int err = 0; socklen_t elen = sizeof(err);
            getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &elen);
            if (err != 0) {
                gcs_log("%s: connect error: %s", name_.c_str(), strerror(err));
                close_socket(fd_);
                fd_ = -1;
                status_.store(LinkStatus::Error);
                return false;
            }
        } else if (rc != 0) {
            gcs_log("%s: connect failed", name_.c_str());
            close_socket(fd_);
            fd_ = -1;
            status_.store(LinkStatus::Error);
            return false;
        }
        // Restore blocking with a 200 ms receive timeout.
        fcntl(fd_, F_SETFL, flags);
        timeval tv{0, 200'000};
        setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        timeval stv{0, 500'000};
        setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &stv, sizeof(stv));
#endif
        is_stream_ = true;
        return true;
    }

    // Serial
    ser_ = serial_open(cfg_.device, cfg_.baud);
    if (ser_ == SERIAL_INVALID) {
        gcs_log("failed to open serial port %s", cfg_.device);
        status_.store(LinkStatus::Error);
        return false;
    }
    is_stream_ = true;
    return true;
}

void Link::close()
{
    if (is_serial_) {
        if (ser_ != SERIAL_INVALID) { serial_close(ser_); ser_ = SERIAL_INVALID; }
    } else if (fd_ >= 0) {
        close_socket(fd_);
        fd_ = -1;
    }
}

RxResult Link::read(uint8_t* buf, size_t cap, int& n,
                    sockaddr_in& src, bool& src_valid)
{
    n         = 0;
    src_valid = false;

    if (!is_stream_) {
        sockaddr_in from{};
        socklen_t   from_len = sizeof(from);
        ssize_t r = recvfrom(fd_, (char*)buf, cap, 0,
                             reinterpret_cast<sockaddr*>(&from), &from_len);
        n = (int)r;
        // A datagram socket has no end of stream: a zero-length datagram is odd
        // but harmless, and only a hard error means the socket is gone.
        if (r > 0) {
            src       = from;
            src_valid = true;
            return RxResult::Data;
        }
        if (r == 0) return RxResult::Quiet;
        return net_error_is_transient(last_net_error()) ? RxResult::Quiet
                                                        : RxResult::Failed;
    }

    if (is_serial_) {
        int r = serial_read(ser_, buf, cap);
        n = r;
        // The port is configured VMIN=0/VTIME=2 on POSIX and with an equivalent
        // read timeout on Windows, so a timeout arrives as 0 bytes. A negative
        // result is the device itself reporting trouble — most often that it is
        // no longer there.
        if (r > 0) return RxResult::Data;
        if (r == 0) return RxResult::Quiet;
#ifndef _WIN32
        if (errno == EINTR) return RxResult::Quiet;
#endif
        return RxResult::Failed;
    }

    // recv(), not read(): Winsock sockets are not CRT file descriptors.
    ssize_t r = recv(fd_, (char*)buf, cap, 0);
    n = (int)r;
    // On a stream socket zero means the peer closed cleanly. That is a finished
    // connection, not an idle one.
    if (r > 0) return RxResult::Data;
    if (r == 0) return RxResult::Closed;
    return net_error_is_transient(last_net_error()) ? RxResult::Quiet
                                                    : RxResult::Failed;
}

void Link::flush_to(MavlinkSender& sender, const sockaddr_in* peer)
{
    if (is_serial_) {
        sender.flush_serial(ser_);
    } else if (is_stream_) {
        sender.flush_stream(fd_);
    } else if (peer) {
        sender.flush(fd_, *peer);
    }
    // A UDP vehicle with no peer address yet has nowhere to send; its frames
    // stay queued until its first datagram tells us where it lives.
}
