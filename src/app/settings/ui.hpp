#pragma once
// Port target: src/app/settings/ui.rs
// Purpose: Settings UI (rendering controls, handling user changes, save/cancel).
// Minimal header-only implementation that edits app::settings::Config using Dear ImGui.

#include <string>
#include <vector>
#include <algorithm>

#include "settings.hpp"
#include "store.hpp"

#include "../../../vendor/imgui/imgui.h"

namespace app {
namespace settings {
namespace ui {

struct UiState {
    bool open = false;
    bool dirty = false;
    Config staged; // staged edits
};

// Global singleton UI state
inline UiState& state() {
    static UiState s{};
    return s;
}

// Begin editing: load current config into staged
inline void begin_edit(const Config& cfg) {
    auto& s = state();
    s.staged = cfg;
    s.dirty = false;
    s.open = true;
}

// Discard staged changes
inline void discard_changes() {
    auto& s = state();
    s.dirty = false;
    s.open = false;
}

// Apply staged changes to cfg (returns true if applied)
inline bool apply_changes(Config& cfg) {
    auto& s = state();
    if (!s.open) return false;
    cfg = s.staged;
    s.dirty = false;
    s.open = false;
    return true;
}

// Render Settings UI in a child window or modal. Returns true if user pressed Save.
inline bool render(Config& cfg) {
    auto& s = state();
    if (!s.open) begin_edit(cfg);

    bool saved = false;

    if (ImGui::BeginChild("settings_panel", ImVec2(0, 0), true)) {
        ImGui::TextUnformatted("Settings");
        ImGui::Separator();

        // Folders
        ImGui::TextUnformatted("Folders");
        {
            char buf[512];

            std::snprintf(buf, sizeof(buf), "%s", s.staged.temp_folder.c_str());
            if (ImGui::InputText("Temp folder", buf, sizeof(buf))) {
                s.staged.temp_folder = buf; s.dirty = true;
            }

            std::snprintf(buf, sizeof(buf), "%s", s.staged.extract_folder.c_str());
            if (ImGui::InputText("Extract folder", buf, sizeof(buf))) {
                s.staged.extract_folder = buf; s.dirty = true;
            }

            std::snprintf(buf, sizeof(buf), "%s", s.staged.cache_folder.c_str());
            if (ImGui::InputText("Cache folder", buf, sizeof(buf))) {
                s.staged.cache_folder = buf; s.dirty = true;
            }
        }

        ImGui::Separator();

        // Language
        {
            const char* langs[] = {"auto", "en", "ru"};
            int idx = 0;
            if (s.staged.language == "en") idx = 1;
            else if (s.staged.language == "ru") idx = 2;
            if (ImGui::BeginCombo("Language", langs[idx])) {
                for (int i = 0; i < 3; ++i) {
                    bool sel = (i == idx);
                    if (ImGui::Selectable(langs[i], sel)) {
                        idx = i;
                        s.staged.language = langs[i];
                        s.dirty = true;
                    }
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Separator();

        // Behavior
        {
            bool cache_on = s.staged.cache_on_download;
            if (ImGui::Checkbox("Cache on download", &cache_on)) { s.staged.cache_on_download = cache_on; s.dirty = true; }
            bool log_file = s.staged.log_to_file;
            if (ImGui::Checkbox("Log to file", &log_file)) { s.staged.log_to_file = log_file; s.dirty = true; }
        }

        ImGui::Separator();

        // Launch
        {
            char buf[512];
            std::snprintf(buf, sizeof(buf), "%s", s.staged.custom_launch.c_str());
            if (ImGui::InputText("Custom launch ({{path}})", buf, sizeof(buf))) {
                s.staged.custom_launch = buf; s.dirty = true;
            }
        }

        ImGui::Separator();

        // Startup filters (simple text areas with comma-separated values)
        auto edit_list = [&](const char* label, std::vector<std::string>& v) {
            std::string combined;
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) combined += ", ";
                combined += v[i];
            }
            char buf[1024];
            std::snprintf(buf, sizeof(buf), "%s", combined.c_str());
            if (ImGui::InputTextMultiline(label, buf, sizeof(buf), ImVec2(0, 60))) {
                // split by comma
                std::vector<std::string> out;
                std::string cur;
                for (char c : std::string(buf)) {
                    if (c == ',') {
                        if (!cur.empty()) { out.push_back(cur); cur.clear(); }
                    } else if (c == '\r' || c == '\n') {
                        continue;
                    } else {
                        cur.push_back(c);
                    }
                }
                if (!cur.empty()) out.push_back(cur);
                // trim spaces
                auto trim = [](std::string s) {
                    auto b = s.begin(), e = s.end();
                    while (b != e && std::isspace(static_cast<unsigned char>(*b))) ++b;
                    while (e != b && std::isspace(static_cast<unsigned char>(*(e - 1)))) --e;
                    return std::string(b, e);
                };
                for (auto& x : out) x = trim(x);
                v = std::move(out);
                s.dirty = true;
            }
        };
        ImGui::TextUnformatted("Startup filters (comma-separated):");
        edit_list("Startup tags", s.staged.startup_tags);
        edit_list("Startup exclude tags", s.staged.startup_exclude_tags);
        edit_list("Startup prefixes", s.staged.startup_prefixes);
        edit_list("Startup exclude prefixes", s.staged.startup_exclude_prefixes);

        ImGui::Separator();

        // Warnings
        ImGui::TextUnformatted("Warnings (comma-separated):");
        edit_list("Warn tags", s.staged.warn_tags);
        edit_list("Warn prefixes", s.staged.warn_prefixes);

        ImGui::Separator();

        if (ImGui::Button("Save")) {
            saved = apply_changes(cfg);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            discard_changes();
        }
        if (s.dirty) {
            ImGui::SameLine();
            ImGui::TextUnformatted("(modified)");
        }
    }
    ImGui::EndChild();

    return saved;
}

} // namespace ui
} // namespace settings
} // namespace app
