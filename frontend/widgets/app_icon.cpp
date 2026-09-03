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


#include "app_icon.hpp"

#include <GLFW/glfw3.h>

#include "stb_image.h"

void app_set_window_icon(GLFWwindow* window,
                         const std::vector<std::string>& png_paths)
{
    if (!window || png_paths.empty()) return;

    std::vector<GLFWimage>      images;
    std::vector<unsigned char*> pixels;
    images.reserve(png_paths.size());
    pixels.reserve(png_paths.size());

    for (const std::string& path : png_paths) {
        int w = 0, h = 0, ch = 0;
        // 4 channels forced: GLFW wants tightly packed RGBA whatever the file
        // happened to store.
        unsigned char* px = stbi_load(path.c_str(), &w, &h, &ch, 4);
        if (!px) continue;
        pixels.push_back(px);
        images.push_back(GLFWimage{ w, h, px });
    }

    if (!images.empty())
        glfwSetWindowIcon(window, (int)images.size(), images.data());

    // GLFW copies the pixels into its own storage, so the decoded buffers are
    // ours to release the moment the call returns.
    for (unsigned char* px : pixels) stbi_image_free(px);
}
