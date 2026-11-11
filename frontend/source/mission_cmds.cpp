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

// frontend/source/mission_cmds.cpp

#include "frontend/source/mission_cmds.hpp"
#include <cstdio>

const MissionCmdMeta* mission_cmd_meta(uint16_t cmd)
{
    for (int i = 0; i < MISSION_CMD_COUNT; ++i)
        if (MISSION_CMDS[i].id == cmd) return &MISSION_CMDS[i];
    return nullptr;
}

const char* mission_cmd_name(uint16_t cmd)
{
    const MissionCmdMeta* m = mission_cmd_meta(cmd);
    if (m) return m->name;
    static char buf[12];
    snprintf(buf, sizeof(buf), "CMD_%u", cmd);
    return buf;
}
