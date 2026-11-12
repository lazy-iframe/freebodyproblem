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

// frontend/components/others/file_dialog.cpp

#include "frontend/components/others/file_dialog.hpp"
#include "frontend/components/others/ui_kit.hpp"
#include "frontend/source/theme.hpp"
#include "imgui.h"
#include "ImGuiFileDialog.h"

// ConfirmOverwrite only on the save side: it is the only one that writes, and
// warning about an overwrite while picking a file to *read* would be asking
// about something that is not going to happen.
void file_dialog_open(const FileDialogSpec& spec, const std::string& last_path)
{
    IGFD::FileDialogConfig cfg;
    if (!last_path.empty())
        cfg.filePathName = last_path;   // IGFD splits this into path + name
    else
        cfg.fileName = spec.default_name;

    cfg.flags = ImGuiFileDialogFlags_Modal;
    if (spec.for_save)
        cfg.flags |= ImGuiFileDialogFlags_ConfirmOverwrite;

    ImGuiFileDialog::Instance()->OpenDialog(spec.key, spec.title, spec.filters, cfg);
}

// Display() returns at once for a key that is not the open dialog, so polling
// every frame costs nothing for the ones that are shut.
bool file_dialog_poll(const char* key, std::string& path_out)
{
    bool ok = false;
    // The app's popup frame, without the dialog style's padding and spacing —
    // those are tuned for a centred title and a button row, and would leave the
    // browser's table looking sparse. apply_global_theme() already squared off
    // every corner.
    ui_push_popup_frame();
    if (ImGuiFileDialog::Instance()->Display(key, ImGuiWindowFlags_NoCollapse,
                                             { 640.0f, 420.0f })) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            path_out = ImGuiFileDialog::Instance()->GetFilePathName();
            ok = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }
    ui_pop_popup_frame();
    return ok;
}
