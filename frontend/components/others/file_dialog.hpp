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

// frontend/components/others/file_dialog.hpp
//
// The app's file browser: a modal for naming a file to read or write, in the
// app's own colours. A wrapper over ImGuiFileDialog, which nothing else
// includes. Open it from a button, then poll it every frame with the same key;
// the poll hands back the path once, when the operator confirms. Only one can
// be open at a time, since ImGuiFileDialog holds a single dialog's state.

#pragma once

#include <string>

struct FileDialogSpec {
    // The key the dialog is tracked under. One per purpose, so each carries
    // its own title and flags.
    //
    // Deliberately without the "##" prefix ImGui ids carry. These are not
    // ImGui labels, they are ImGuiFileDialog's keys, and it names its window
    // `title + "##" + key` (ImGuiFileDialog.cpp). A "##" here would make that
    // four consecutive '#', which ImHashStr and ImHashSkipUncontributingPrefix
    // disagree about: the former consumes one "###" and hashes the leftover '#'
    // as data, the latter re-matches and skips it. The window would then hash to
    // one id and its .ini settings entry to another, and imgui asserts
    // (settings->ID == window->ID) the next time it writes imgui.ini.
    const char* key;
    const char* title;
    // ImGuiFileDialog's form: "Parameter files (*.params){.params},All files{.*}".
    // Null to choose a folder instead of a file: `last_path` is then the folder
    // to start in, and the poll hands back the folder chosen.
    const char* filters;
    const char* default_name;   // the file offered when there is no last path
    bool        for_save;       // asks before overwriting
};

// Opens the browser, seeded with `last_path` (the last file touched, if any)
// so a save after a load starts in the same directory.
void file_dialog_open(const FileDialogSpec& spec, const std::string& last_path);

// Draws the browser under `key` if it is the one open. True once, with the
// chosen file in `path_out`, when the operator confirms; false every other
// frame, and when they cancel.
bool file_dialog_poll(const char* key, std::string& path_out);
