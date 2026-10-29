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


// freebodyproblem-mcp — the GCS's MCP server, over stdio.
//
// The server lives inside the GCS and speaks MCP's Streamable HTTP on
// 127.0.0.1 (backend/mcp_server.hpp). Some MCP hosts can only launch a server
// as a child process and talk to it on stdin/stdout. This is that child: it
// reads one JSON-RPC message per line, POSTs it to the running GCS, and writes
// the reply back as one line. It knows nothing about MCP beyond that — every
// tool, check and approval is the GCS's — so it never needs changing when the
// tools do.
//
// Requests are forwarded concurrently, each on its own thread. They have to
// be: an action can wait most of a minute for the operator, and the client's
// notifications/cancelled for it arrives on stdin while it waits.
//
// stdout carries the protocol and nothing else. Diagnostics go to stderr,
// which hosts show in their logs.

#include "httplib.h"
#include <nlohmann/json.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#  include <fcntl.h>
#  include <io.h>
#endif

using json = nlohmann::json;

namespace {

constexpr int DEFAULT_PORT = 8737;

// The server gives one tools/call at most 55 s (CALL_BUDGET in
// backend/mcp_server.cpp). A little longer here, so the reply is never cut off
// by this side giving up first.
constexpr int READ_TIMEOUT_S = 70;

// JSON-RPC's "server error" range; the code says the GCS itself was not reached.
constexpr int RPC_UNREACHABLE = -32000;

struct Target {
    std::string host = "127.0.0.1";
    int         port = DEFAULT_PORT;
    std::string token;
    bool        token_from_settings = false;
};

// Where the GCS keeps settings.json. Must match settings_path() in
// frontend/settings.cpp, which this repeats rather than links: that file
// brings the theme and ImGui types with it.
std::string settings_path()
{
#ifdef _WIN32
    const char* base = std::getenv("APPDATA");
    if (base) return std::string(base) + "/freebodyproblem/settings.json";
#else
    const char* cfg = std::getenv("XDG_CONFIG_HOME");
    if (cfg && cfg[0]) return std::string(cfg) + "/freebodyproblem/settings.json";
    const char* home = std::getenv("HOME");
    if (home) return std::string(home) + "/.config/freebodyproblem/settings.json";
#endif
    return "./freebodyproblem_settings.json";
}

// Port and token as the GCS has them saved. False when there is no file.
bool read_settings(int& port, std::string& token)
{
    std::ifstream f(settings_path());
    if (!f) return false;
    const json s = json::parse(f, nullptr, false);
    if (!s.is_object()) return false;
    if (s.contains("mcp_port") && s["mcp_port"].is_number_integer()) port = s["mcp_port"].get<int>();
    if (s.contains("mcp_token") && s["mcp_token"].is_string()) token = s["mcp_token"].get<std::string>();
    return true;
}

void usage()
{
    std::fprintf(stderr,
        "usage: freebodyproblem-mcp [--port N] [--token T]\n"
        "\n"
        "Connects an MCP host that speaks stdio to a running Free Body Problem Ground Control Station instance.\n"
        "Port and token come from, in order: these flags, the FBP_MCP_PORT and\n"
        "FBP_MCP_TOKEN environment variables, then the GCS's own settings file\n"
        "(%s).\n"
        "Turn the server on in the GCS under SETTINGS > ASSISTANT (MCP).\n",
        settings_path().c_str());
}

// ── Output ───────────────────────────────────────────────────────────────────

std::mutex g_out_mtx;

// One message, one line, flushed — the whole stdio framing. Re-serialised
// rather than passed through, so a reply is guaranteed to be a single line.
void emit(const json& msg)
{
    const std::string line = msg.dump();
    std::lock_guard<std::mutex> lk(g_out_mtx);
    std::fwrite(line.data(), 1, line.size(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

void emit_error(const json& id, const std::string& message)
{
    emit({ {"jsonrpc", "2.0"}, {"id", id},
           {"error", { {"code", RPC_UNREACHABLE}, {"message", message} }} });
}

// ── Forwarding ───────────────────────────────────────────────────────────────

class Bridge {
public:
    explicit Bridge(Target t) : target_(std::move(t)) {}

    // Forwards one line and writes the reply, if there is one. Thread-safe.
    void forward(const std::string& line)
    {
        const json msg = json::parse(line, nullptr, false);
        // Only requests are answered; for a notification the host expects
        // nothing back, error or otherwise.
        const bool is_request = msg.is_object() && msg.contains("id") && msg.contains("method");
        const json id = is_request ? msg["id"] : json();

        httplib::Result res = post(line);

        // A new token from the GCS's NEW TOKEN button: read it again and retry
        // once, so the host does not have to be restarted for it.
        if (res && res->status == 401 && target_.token_from_settings && reload_token())
            res = post(line);

        if (!res) {
            std::fprintf(stderr, "freebodyproblem-mcp: %s:%d unreachable (%s)\n",
                         target_.host.c_str(), target_.port,
                         httplib::to_string(res.error()).c_str());
            if (is_request)
                emit_error(id, "Free Body Problem is not reachable on " + target_.host + ":" +
                               std::to_string(target_.port) + ". Is the GCS running, with "
                               "SETTINGS > ASSISTANT (MCP) > MCP server on?");
            return;
        }

        // The server hands out a session at initialize; every later message
        // carries it, which is what lets a cancellation find its request.
        if (res->has_header("Mcp-Session-Id")) {
            std::lock_guard<std::mutex> lk(mtx_);
            session_ = res->get_header_value("Mcp-Session-Id");
        }

        if (res->status == 202) return;                      // a notification, accepted
        if (res->status == 401) {
            if (is_request) emit_error(id, "The GCS refused the token. Copy it again from "
                                           "SETTINGS > ASSISTANT (MCP).");
            return;
        }
        if (res->status != 200) {
            std::fprintf(stderr, "freebodyproblem-mcp: HTTP %d\n", res->status);
            if (is_request) emit_error(id, "The GCS answered HTTP " + std::to_string(res->status) + ".");
            return;
        }

        const json reply = json::parse(res->body, nullptr, false);
        if (reply.is_discarded()) {
            if (is_request) emit_error(id, "The GCS sent a reply that is not JSON.");
            return;
        }
        emit(reply);
    }

private:
    httplib::Result post(const std::string& body)
    {
        // A client per request: httplib's is not safe to share between
        // threads, and on loopback a connection costs next to nothing.
        httplib::Client cli(target_.host, target_.port);
        cli.set_connection_timeout(3, 0);
        cli.set_read_timeout(READ_TIMEOUT_S, 0);

        httplib::Headers h;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (!target_.token.empty()) h.emplace("Authorization", "Bearer " + target_.token);
            if (!session_.empty())      h.emplace("Mcp-Session-Id", session_);
        }
        return cli.Post("/mcp", h, body, "application/json");
    }

    bool reload_token()
    {
        int port = 0;
        std::string token;
        if (!read_settings(port, token) || token.empty()) return false;
        std::lock_guard<std::mutex> lk(mtx_);
        if (token == target_.token) return false;
        target_.token = token;
        std::fprintf(stderr, "freebodyproblem-mcp: token changed in settings; using the new one\n");
        return true;
    }

    Target      target_;
    std::mutex  mtx_;          // token and session
    std::string session_;
};

} // namespace

int main(int argc, char** argv)
{
#ifdef _WIN32
    // Text mode would turn each "\n" written into "\r\n" and strip the "\r"
    // read, neither of which the host is expecting.
    _setmode(_fileno(stdin),  _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    Target t;
    int         flag_port = 0;
    std::string flag_token;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if ((a == "--port" || a == "-p") && i + 1 < argc)  flag_port  = std::atoi(argv[++i]);
        else if ((a == "--token" || a == "-t") && i + 1 < argc) flag_token = argv[++i];
        else if (a == "--help" || a == "-h") { usage(); return 0; }
        else { std::fprintf(stderr, "freebodyproblem-mcp: unknown argument %s\n", a.c_str()); usage(); return 2; }
    }

    int         saved_port = 0;
    std::string saved_token;
    read_settings(saved_port, saved_token);

    const char* env_port  = std::getenv("FBP_MCP_PORT");
    const char* env_token = std::getenv("FBP_MCP_TOKEN");
    if (flag_port > 0)                 t.port = flag_port;
    else if (env_port && *env_port)    t.port = std::atoi(env_port);
    else if (saved_port > 0)           t.port = saved_port;
    if (!flag_token.empty())           t.token = flag_token;
    else if (env_token && *env_token)  t.token = env_token;
    else { t.token = saved_token; t.token_from_settings = true; }

    if (t.port <= 0 || t.port > 65535) {
        std::fprintf(stderr, "freebodyproblem-mcp: bad port %d\n", t.port);
        return 2;
    }
    std::fprintf(stderr, "freebodyproblem-mcp: forwarding to http://%s:%d/mcp\n",
                 t.host.c_str(), t.port);

    Bridge bridge(std::move(t));

    // In-flight requests, so end of input waits for their replies instead of
    // cutting them off — a host closes stdin when it is done with us.
    std::mutex              inflight_mtx;
    std::condition_variable inflight_cv;
    int                     inflight = 0;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find_first_not_of(" \t") == std::string::npos) continue;
        {
            std::lock_guard<std::mutex> lk(inflight_mtx);
            ++inflight;
        }
        std::thread([&bridge, &inflight_mtx, &inflight_cv, &inflight, line] {
            bridge.forward(line);
            std::lock_guard<std::mutex> lk(inflight_mtx);
            --inflight;
            inflight_cv.notify_all();
        }).detach();
    }

    std::unique_lock<std::mutex> lk(inflight_mtx);
    inflight_cv.wait(lk, [&] { return inflight == 0; });
    return 0;
}
