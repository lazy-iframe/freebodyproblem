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

class McpServer;

// The operator's half of an MCP action: a modal naming what the assistant wants
// done, to which vehicle and why, with ALLOW and REFUSE. One request at a time,
// oldest first. Call once per frame from the UI thread; an approved action runs
// inside this call.
void draw_mcp_approval(McpServer& server);

// Whether the MCP server is up and listening, for the SETTINGS tab to say so.
// Defined beside the server, in main.cpp.
bool gcs_mcp_listening();

// The vehicles agents have claimed, and the operator's way to clear them all.
// Also defined in main.cpp.
#include <vector>
struct McpClaim;
std::vector<McpClaim> gcs_mcp_claims();
void                  gcs_mcp_release_claims();

// Full path of the stdio adapter installed beside the GCS — what an MCP host is
// given as its command — or empty when it is not there. Also in main.cpp.
#include <string>
const std::string& gcs_mcp_adapter_path();
