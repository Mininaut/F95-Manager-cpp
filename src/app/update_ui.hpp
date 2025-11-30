#pragma once
// Port target: src/app/update_ui.rs
// Purpose: UI for checking/downloading/updating application versions.
// Minimal header-only implementation that shows a modal about updates.
// Real network/versioning can be wired later via app::fetch.

#include <string>
#include <utility>

#include "../../vendor/imgui/imgui.h"
#include "../logger.hpp"
#include "settings/helpers/open.hpp"

namespace app {
namespace update_ui {

struct UpdateState {
    bool dialog_open = false;
    std::string current_version;
    std::string latest_version;
};

// Global update UI state
inline UpdateState& state() {
    static UpdateState s;
    return s;
}

// Begin an update check (stub). In a real impl, fetch latest version and compare.
// For parity in this port, we simply take provided strings and open dialog.
inline void check_for_updates(const std::string& current_version = "0.0.0",
                              const std::string& latest_version  = "0.0.0") {
    auto& st = state();
    st.current_version = current_version;
    st.latest_version = latest_version;
    st.dialog_open = true;
    logger::info(std::string("Update check: current=") + current_version + " latest=" + latest_version);
}

// Show modal dialog with update info. Call every frame while your frame is active.
inline void show_update_dialog() {
    auto& st = state();
    if (!st.dialog_open) return;

    ImGui::OpenPopup("Update");
    bool open = true;
    if (ImGui::BeginPopupModal("Update", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("Current version: %s", st.current_version.c_str());
        ImGui::Text("Latest version:  %s", st.latest_version.c_str());
        bool newer = (st.latest_version != st.current_version && !st.latest_version.empty());
        ImGui::Separator();
        if (newer) {
            ImGui::TextUnformatted("A newer version is available.");
            if (ImGui::Button("Open download page")) {
                const char* rel_url = "https://example.com/releases";
                (void)app::settings::helpers::open::url(rel_url);
                logger::info(std::string("Opening download page: ") + rel_url);
            }
            ImGui::SameLine();
        } else {
            ImGui::TextUnformatted("You are up to date.");
        }
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            st.dialog_open = false;
        }
        ImGui::EndPopup();
    }
    if (!open) {
        st.dialog_open = false;
    }
}

} // namespace update_ui
} // namespace app
