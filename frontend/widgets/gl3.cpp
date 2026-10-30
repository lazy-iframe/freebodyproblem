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


#include "gl3.hpp"
#include <GLFW/glfw3.h>

namespace gl3 {

#define GL3_DEFINE(ret, name, args) ret (GL3_API* name) args = nullptr;
GL3_FUNCS(GL3_DEFINE)
#undef GL3_DEFINE

bool load()
{
    static int s_state = 0;   // 0 untried, 1 loaded, -1 something missing
    if (s_state) return s_state > 0;

    bool ok = true;
#define GL3_LOAD(ret, name, args)                                                       \
    name = reinterpret_cast<ret (GL3_API*) args>(glfwGetProcAddress("gl" #name));       \
    ok   = ok && name != nullptr;
    GL3_FUNCS(GL3_LOAD)
#undef GL3_LOAD

    s_state = ok ? 1 : -1;
    return ok;
}

} // namespace gl3
