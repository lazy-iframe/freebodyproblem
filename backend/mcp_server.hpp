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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Fleet;
class Vehicle;

// A Model Context Protocol server, so any agent or client that speaks MCP can
// read the fleet and act on it.
//
// Transport is MCP's Streamable HTTP, served on loopback only: the GCS is a GUI
// the operator already has running, so a client connects to it rather than
// launching it, which rules out stdio. Every request is a POST of one JSON-RPC
// message to /mcp and every reply is one JSON body; the optional server-sent
// event stream is not offered, which the spec allows.
//
// Reading is free. What an assistant may *do* is the operator's call, made in
// the SETTINGS tab: actions as a whole are on or off, and each one either runs
// straight away or waits for the operator to ALLOW it on screen. Whichever,
// every action runs on the UI thread, in McpServer::pump() or in the dialog.
//
// Threading: the HTTP server runs its own threads. Reads go through
// Fleet::vehicles() and Vehicle::snapshot(), both safe from any thread. The
// action queue and the policy are the only state shared with the UI thread.
struct McpConfig {
    int         port = 8737;
    std::string token;   // required as "Authorization: Bearer …"; empty disables the check
};

// One thing an assistant can do to a vehicle, as the SETTINGS tab lists it.
struct McpActionInfo {
    const char* tool;    // the MCP tool name, and the key in McpPolicy::auto_allow
    const char* label;   // "Arm"
    const char* hint;    // one line on what it sends
};

// Every action tool, in the order the SETTINGS tab shows them.
const std::vector<McpActionInfo>& mcp_actions();

struct McpPolicy {
    bool                        actions_enabled = false;  // false: action tools are not even listed
    std::map<std::string, bool> auto_allow;               // tool → runs without asking; absent asks
    std::string                 instructions;             // the operator's rules for agents; may be empty

    bool operator==(const McpPolicy& o) const
    {
        return actions_enabled == o.actions_enabled && auto_allow == o.auto_allow &&
               instructions == o.instructions;
    }
    bool operator!=(const McpPolicy& o) const { return !(*this == o); }
};

// One action waiting for the operator, as the approval dialog shows it.
struct McpApproval {
    uint64_t    id = 0;
    std::string summary;   // one line: "SET MODE → GUIDED"
    std::string target;    // which vehicle: "(2) sys1 on udp:0.0.0.0:14550"
    std::string reason;    // what the assistant said it is for; may be empty
    std::string warning;   // a separation concern the operator should weigh; may be empty
};

// One line for the fleet event log: what an agent did through MCP, and how it
// went. `severity` is a MAV_SEVERITY, so the log colours it like a vehicle's
// own message — 2 and below red, 3 orange, 4 amber, 5 accent, 6 plain.
struct McpEvent {
    uint8_t     severity = 6;
    std::string text;
};

// A vehicle an agent has claimed, as the SETTINGS tab lists it.
struct McpClaim {
    uint32_t    vehicle = 0;   // the GCS's number for it, "(2)"
    std::string label;         // who claimed it, as the agent named itself
    double      idle_s  = 0;   // since the claim was last used
};

class McpServer {
public:
    // Runs a plugin against one vehicle. Supplied by the frontend, which is
    // what knows how to build a PluginContext; returns false when the slot
    // does not exist.
    using PluginRunner = std::function<bool(int slot, Vehicle& v)>;

    explicit McpServer(Fleet& fleet);
    ~McpServer();

    McpServer(const McpServer&)            = delete;
    McpServer& operator=(const McpServer&) = delete;

    // Binds 127.0.0.1:port and starts serving. False (and logged) when the port
    // is taken. Calling start() while running restarts with the new config.
    bool start(const McpConfig& cfg);
    void stop();                               // idempotent; refuses anything still pending
    bool running() const { return running_.load(); }

    // Safe from any thread, and takes effect on the next call: the SETTINGS tab
    // can change it under a connected assistant.
    void      set_policy(McpPolicy p);
    McpPolicy policy() const;

    void set_plugin_runner(PluginRunner fn) { plugin_runner_ = std::move(fn); }

    // Live claims, and the operator's way to clear them. A claim only stops
    // other agents; it never stands between the operator and a vehicle.
    std::vector<McpClaim> claims() const;
    void                  release_all_claims();

    // Everything agents have done since the last call, oldest first, for the
    // fleet event log. UI thread, once per frame.
    std::vector<McpEvent> take_events();

    // ── UI thread ─────────────────────────────────────────────────────────────

    // Runs the queued actions that need nobody's say-so. Once per frame.
    void pump();

    // The oldest action awaiting a decision. False when there is none.
    bool pending_approval(McpApproval& out) const;

    // The operator's answer. On approval the action runs here, on the calling
    // thread — which must be the UI thread, since plugins assume it.
    void resolve_approval(uint64_t id, bool approve);

private:
    struct Pending;

    // Queues an action for the UI thread — for the operator to answer, or for
    // pump() to run, as the policy says for `tool` — and blocks the calling
    // HTTP thread until it has run, been turned down, or `deadline` passes.
    // Returns the action's own text, or why it did not run; `ok` false for the
    // latter. `rpc_key` names the request so the client can cancel it.
    std::string request_action(const std::string& tool, const std::string& rpc_key,
                               std::chrono::steady_clock::time_point deadline,
                               McpApproval info, std::function<std::string()> action,
                               bool& ok);

    // The client's notifications/cancelled. Withdraws the request if it has not
    // run yet — the dialog closes and nothing is sent. Too late otherwise.
    void cancel_request(const std::string& rpc_key);

    void        finish(const std::shared_ptr<Pending>& p);   // runs it, wakes the waiter

    // Queue a line for the event log. Any thread.
    void event(uint8_t severity, std::string text);

    // A read, for the event log — throttled: see READ_LOG_INTERVAL.
    void event_read(const std::string& what);
    // One JSON-RPC message in, one reply out. `session` is the client's
    // Mcp-Session-Id, and is set to a new one by initialize.
    std::string handle(const std::string& body, std::string& session, int& http_status);

    Fleet&       fleet_;
    McpConfig    cfg_;
    PluginRunner plugin_runner_;

    mutable std::mutex policy_mtx_;
    McpPolicy          policy_;

    struct Impl;
    std::unique_ptr<Impl> impl_;            // the httplib server, kept out of this header
    std::thread           thread_;
    std::atomic<bool>     running_{false};

    // Coordination between agents, keyed by vehicle (link id and sysid packed
    // into one integer). A claim reserves a vehicle for whoever holds its
    // token; a target is the last point an agent sent a vehicle to, kept so
    // a second agent's path can be checked against where the first is going.
    struct Claim {
        std::string token, label;
        uint32_t    number = 0;
        std::chrono::steady_clock::time_point used;
    };
    struct Target { double lat, lon, amsl; };
    mutable std::mutex         coord_mtx_;
    std::map<uint64_t, Claim>  claims_;
    std::map<uint64_t, Target> targets_;

    // Event-log lines waiting for the UI thread, and the read throttle's
    // counts: per tool and vehicle, how many since the last line, and when
    // that was.
    struct ReadTally { std::chrono::steady_clock::time_point last; int count = 0; };
    std::mutex                       events_mtx_;
    std::deque<McpEvent>             events_;
    std::map<std::string, ReadTally> read_tally_;

    mutable std::mutex                    queue_mtx_;
    std::condition_variable               queue_cv_;
    std::deque<std::shared_ptr<Pending>>  queue_;
    uint64_t                              next_id_ = 1;

    friend struct McpTools;
};

// A fresh random token, 32 hex characters, for first-time setup.
std::string mcp_generate_token();
