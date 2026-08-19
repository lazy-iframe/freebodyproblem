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


#include "plugin_api.hpp"

#include "../frontend/app_log.hpp"

#include <exception>

// The registry proper. Not a namespace-scope vector: registrars are static
// objects in other translation units, and their constructors run in an order
// the standard leaves unspecified — one of them running before a plain global
// vector was constructed would push into raw memory. A function-local static is
// built on first use, which is by definition the first registration.

static std::vector<PluginSlot>& registry()
{
    static std::vector<PluginSlot> slots;
    return slots;
}

// Startup hooks, held by the plugin they belong to rather than by slot index:
// a hook's registrar may well run before the registrar of the plugin it names,
// so there is no index to record yet. Resolved in gcs_plugins_init().
struct PendingInit {
    PluginFn     owner;
    PluginInitFn init;
};

static std::vector<PendingInit>& init_hooks()
{
    static std::vector<PendingInit> hooks;
    return hooks;
}

PluginRegistrar::PluginRegistrar(const char* tag, const char* label,
                                 const char* tooltip, PluginFn fn)
{
    if (!fn) return;
    registry().push_back(PluginSlot{ tag     ? tag     : "?",
                                     label   ? label   : "",
                                     tooltip ? tooltip : "",
                                     false,
                                     fn });
}

const std::vector<PluginSlot>& gcs_plugins()
{
    return registry();
}

PluginInitRegistrar::PluginInitRegistrar(PluginFn owner, PluginInitFn init)
{
    if (!owner || !init) return;
    init_hooks().push_back(PendingInit{ owner, init });
}

void gcs_plugins_init()
{
    // Guarded rather than trusted: the app calls this once, but a plugin that
    // reaches for it would otherwise re-run every hook.
    static bool done = false;
    if (done) return;
    done = true;

    const std::vector<PluginSlot>& slots = registry();

    // Slot order, not registration order, so hooks run top-to-bottom down the
    // rail — the order a reader of the rail would expect them in.
    for (size_t i = 0; i < slots.size(); ++i) {
        for (const PendingInit& p : init_hooks()) {
            if (p.owner != slots[i].fn) continue;
            p.init(PluginInit{ (int)i });
        }
    }

    // A hook naming a plugin that was never registered is a typo in the ident,
    // and silently doing nothing is exactly how that typo survives.
    for (const PendingInit& p : init_hooks()) {
        bool owned = false;
        for (const PluginSlot& s : slots)
            if (s.fn == p.owner) { owned = true; break; }
        if (!owned)
            gcs_log("plugin init hook has no matching GCS_PLUGIN — ignored");
    }
}

// ── Renaming ─────────────────────────────────────────────────────────────────
//
// Slots own their strings rather than pointing at the literals passed to the
// macro, precisely so a plugin can hand in a name built at runtime and not have
// to keep the buffer alive.

static PluginSlot* slot_at(int slot)
{
    std::vector<PluginSlot>& r = registry();
    if (slot < 0 || (size_t)slot >= r.size()) return nullptr;
    return &r[(size_t)slot];
}

void gcs_plugin_set_label_at(int slot, const char* label)
{
    if (PluginSlot* s = slot_at(slot)) s->label = label ? label : "";
}

void gcs_plugin_set_tag_at(int slot, const char* tag)
{
    // An empty tag would draw an unlabelled plate, which reads as a bug rather
    // than a choice, so it keeps the tag it had.
    if (PluginSlot* s = slot_at(slot))
        if (tag && *tag) s->tag = tag;
}

void gcs_plugin_set_tooltip_at(int slot, const char* tooltip)
{
    if (PluginSlot* s = slot_at(slot)) s->tooltip = tooltip ? tooltip : "";
}

void gcs_plugin_set_label(const PluginContext& ctx, const char* label)
{
    gcs_plugin_set_label_at(ctx.slot, label);
}

void gcs_plugin_set_tag(const PluginContext& ctx, const char* tag)
{
    gcs_plugin_set_tag_at(ctx.slot, tag);
}

void gcs_plugin_set_tooltip(const PluginContext& ctx, const char* tooltip)
{
    gcs_plugin_set_tooltip_at(ctx.slot, tooltip);
}

void gcs_plugin_set_label(const PluginInit& init, const char* label)
{
    gcs_plugin_set_label_at(init.slot, label);
}

void gcs_plugin_set_tag(const PluginInit& init, const char* tag)
{
    gcs_plugin_set_tag_at(init.slot, tag);
}

void gcs_plugin_set_tooltip(const PluginInit& init, const char* tooltip)
{
    gcs_plugin_set_tooltip_at(init.slot, tooltip);
}

void gcs_plugin_set_engaged_at(int slot, bool engaged)
{
    if (PluginSlot* s = slot_at(slot)) s->engaged = engaged;
}

bool gcs_plugin_engaged_at(int slot)
{
    const PluginSlot* s = slot_at(slot);
    return s && s->engaged;
}

void gcs_plugin_set_engaged(const PluginContext& ctx, bool engaged)
{
    gcs_plugin_set_engaged_at(ctx.slot, engaged);
}

void gcs_plugin_set_engaged(const PluginInit& init, bool engaged)
{
    gcs_plugin_set_engaged_at(init.slot, engaged);
}

// ── Video event handlers ─────────────────────────────────────────────────────
//
// Two more registration lists, same static-init discipline as the buttons: a
// function-local static, built on first use, so registrar order cannot matter.
// Handlers carry their own name only so a throw can be reported against
// something the author will recognise.

template <typename Fn>
struct VideoHandler {
    const char* name;
    Fn          fn;
};

static std::vector<VideoHandler<PluginVideoClickFn>>& click_handlers()
{
    static std::vector<VideoHandler<PluginVideoClickFn>> hs;
    return hs;
}

static std::vector<VideoHandler<PluginVideoBoxFn>>& box_handlers()
{
    static std::vector<VideoHandler<PluginVideoBoxFn>> hs;
    return hs;
}

VideoClickRegistrar::VideoClickRegistrar(const char* name, PluginVideoClickFn fn)
{
    if (fn) click_handlers().push_back({ name ? name : "?", fn });
}

VideoBoxRegistrar::VideoBoxRegistrar(const char* name, PluginVideoBoxFn fn)
{
    if (fn) box_handlers().push_back({ name ? name : "?", fn });
}

bool gcs_video_has_click_handlers() { return !click_handlers().empty(); }
bool gcs_video_has_box_handlers()   { return !box_handlers().empty(); }

// One handler throwing does not cancel the others: the operator made a single
// gesture, and a broken listener is no reason for the working ones to miss it.
void gcs_video_dispatch_click(const PluginContext& ctx, const VideoClick& click)
{
    for (const auto& h : click_handlers()) {
        try {
            h.fn(ctx, click);
        } catch (const std::exception& e) {
            gcs_log("video click handler %s threw: %s", h.name, e.what());
        } catch (...) {
            gcs_log("video click handler %s threw", h.name);
        }
    }
}

void gcs_video_dispatch_box(const PluginContext& ctx, const VideoBox& box)
{
    for (const auto& h : box_handlers()) {
        try {
            h.fn(ctx, box);
        } catch (const std::exception& e) {
            gcs_log("video box handler %s threw: %s", h.name, e.what());
        } catch (...) {
            gcs_log("video box handler %s threw", h.name);
        }
    }
}
