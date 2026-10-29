---
title: AI agents (MCP)
description: Let an AI agent read the fleet and, if you allow it, command vehicles — with you approving what it does.
---

The GCS has a built-in [Model Context Protocol](https://modelcontextprotocol.io)
server, so any agent or LLM host that speaks MCP can connect to it: Claude Code,
opencode, Claude Desktop, Cursor, or your own scripts. That includes small models
running locally. The agent can read every vehicle's telemetry, parameters,
mission and messages. If you turn actions on, it can also command vehicles,
within limits you set and normally with you approving each command on screen.

Nothing is reachable until you switch it on, and actions stay off until you
switch those on separately.

## Turning it on

1. Open the **SETTINGS** tab and scroll to **ASSISTANT (MCP)**
2. Tick **MCP server**. The line beside it should read
   `listening on 127.0.0.1:8737`
3. A token is generated the first time. **COPY TOKEN** copies it, and
   **NEW TOKEN** replaces it, which disconnects every client set up with the
   old one

The server only listens on this computer (`127.0.0.1`), and every request has
to carry the token. Requests from web pages on other sites are refused, so a
website open in your browser cannot reach it either.

## Connecting an agent

Most hosts start an MCP server as a program and talk to it over stdin and
stdout. That program is `freebodyproblem-mcp`, the adapter installed beside the
GCS. It forwards everything to the running GCS, and finds the port and token by
itself in the GCS's `settings.json`.

**COPY ADAPTER PATH**, in the same settings section, copies its full path:

| Installed from | Adapter |
|---|---|
| Linux `.deb` | `/usr/bin/freebodyproblem-mcp` (on `PATH`) |
| Windows installer | `C:\Program Files\freebodyproblem\bin\freebodyproblem-mcp.exe` |
| Source build | `build/freebodyproblem-mcp` |

Give that path to your host as the server's command. For example:

**Claude Code**

```sh
claude mcp add freebodyproblem -- /usr/bin/freebodyproblem-mcp
```

**opencode** (`~/.config/opencode/opencode.json`)

```json
{
  "mcp": {
    "freebodyproblem": {
      "type": "local",
      "command": ["/usr/bin/freebodyproblem-mcp"],
      "enabled": true,
      "codemode": false
    }
  }
}
```

`"codemode": false` matters for small models. With Code Mode on, opencode
makes the model write JavaScript to call tools, and small models get that
wrong. It has to be the boolean `false`, not a string. After changing the
config, restart opencode's background service.

**Other hosts** (Claude Desktop, Cursor and most others use this shape)

```json
{
  "mcpServers": {
    "freebodyproblem": { "command": "/usr/bin/freebodyproblem-mcp" }
  }
}
```

**Over HTTP directly**: hosts that support MCP's Streamable HTTP can skip the
adapter and use `http://127.0.0.1:8737/mcp` with the header
`Authorization: Bearer <token>`.

The adapter also takes `--port` and `--token`, or the environment variables
`FBP_MCP_PORT` and `FBP_MCP_TOKEN`, for a GCS running with a different settings
file. If the GCS is not running, the agent gets a clear "not reachable" error
rather than a hang.

## What an agent can do

**Reading** is always allowed and changes nothing:

| Tool | What it gives |
|---|---|
| `get_guidelines` | How the GCS works, your rules for agents, and what is allowed right now |
| `list_vehicles` | Every vehicle: its number, type, armed state, mode, link and claim |
| `get_telemetry` | Position, home, attitude, speeds, GPS, battery, estimator health, modes |
| `get_params` | Parameters by name or prefix, from the table the GCS has fetched |
| `get_mission` | The mission and geofence, optionally downloaded fresh from the vehicle |
| `get_messages` | The vehicle's recent status texts: prearm failures, warnings |
| `list_plugins` | The buttons on the plugin rail |

**Actions** send commands, and only exist once you tick **Allow actions**:

| Tool | What it does |
|---|---|
| `arm` / `disarm` | Start or stop the motors |
| `takeoff` | Climb to a height above home |
| `goto` | Fly to a point and hold — by latitude/longitude, or as metres north/east/up from where the vehicle is now |
| `set_mode` | Any mode the vehicle accepts |
| `start_mission` / `pause_mission` | Fly the stored mission, or stop and hold where it is |
| `upload_mission` | Replace the stored mission with a list of steps |
| `return_to_launch` | Fly home |
| `set_param` | Change one parameter |
| `run_plugin` | Press a plugin button for a vehicle |

Vehicles are named by the GCS's own number, the `(2)` on the map and callsign
chip, not by MAVLink system ID. Altitudes are metres above home. Every action
reports whether the vehicle accepted it, and points the agent at telemetry to
check what actually happened.

The `goto` offset form exists for agents that think in "100 m north": the GCS
does the coordinate arithmetic rather than the model, which small models
frequently get wrong.

## Approving actions

Under **Allow actions**, each action has a **Run without asking** box.

- **Unticked** (the default for every action): when an agent asks, an
  **ASSISTANT REQUEST** dialog opens with what it wants to do, to which
  vehicle, and the reason it gave. **ALLOW** sends it; **REFUSE** or
  **ESC** turns it down. ALLOW has no keyboard shortcut, on purpose: the
  dialog opens on its own, and an Enter meant for something you were typing
  must not approve a command. If you do not answer within 45 seconds, the
  request is refused.
- **Ticked**: the action runs as soon as the agent asks. It is still
  written to the event log.

**ASK FOR ALL** and **ALLOW ALL** set every box at once. Changes take effect
immediately, even for an agent that is already connected. Unticking **Allow
actions** removes the action tools from it straight away.

## Your rules for agents

**Instructions for agents** is a free-text box sent to every agent when it
connects, after the GCS's own guidance, and again whenever an agent calls
`get_guidelines`. Use it for site rules: a ceiling, an area to keep out of,
which vehicles an agent may touch. Agents are told to follow it, but it is
guidance to the model, not something the GCS enforces. Use the approval
dialog for anything you need to be certain about.

## Several agents at once

When more than one agent shares the fleet, an agent can **claim** a vehicle
with `claim_vehicle`. While the claim holds, other agents' actions on that
vehicle are refused. A claim ends when the agent releases it or after 15
minutes without use. Claims only ever stop other agents, never you. The
settings section lists the current claims, and **RELEASE ALL CLAIMS** clears
them.

## Separation warnings

Before a `takeoff` or `goto`, the GCS walks the path the vehicle will fly and
checks it against every other armed vehicle and every point another agent has
sent a vehicle to. If they come within 10 m horizontally and 5 m vertically, the
approval dialog shows a red **SEPARATION** warning, and the agent is told. The
check is advisory: it does not block the command, you decide.

## In the event log

Everything agents do appears in the event log, tagged **MCP** in blue: an agent
connecting, every action with its outcome, and refusals. Reads are logged too,
but grouped, at most one line per tool and vehicle every 30 seconds. An agent
polling telemetry cannot push the vehicles' own messages out of the log.
