#pragma once
// Port target: src/app/about_ui.rs
// Purpose: About dialog/window UI glue (ImGui modal)

#include <string>
#include "../localization/mod.hpp"
#include "../../vendor/imgui/imgui.h"

namespace app {
namespace about_ui {

// Shows a simple About modal window.
// - open: toggled by caller; when true the modal will open, and will be set false on close.
// - bundle: for localized strings if available.
inline void show_about_dialog(bool& open, localization::Bundle& bundle) {
    if (open) {
        ImGui::OpenPopup("About");
        open = false; // will be controlled by the modal itself after opening
    }

    bool show = true;
    if (ImGui::BeginPopupModal("About", &show, ImGuiWindowFlags_AlwaysAutoResize)) {
        const char* title = "F95 Manager";
        const char* version = "C++ Port";
        // Basic information block; keys can be localized later if added to .ftl
        ImGui::Text("%s", title);
        ImGui::Separator();
        ImGui::Text("Version: %s", version);
        ImGui::Text("This is a native C++ port targeting visual/UX parity with the original.");
        ImGui::Text("Render: Dear ImGui + D3D11 (Win32)");

        ImGui::Dummy(ImVec2(1, 8));
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace about_ui
} // namespace app
