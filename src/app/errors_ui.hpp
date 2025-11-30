#pragma once
// Port target: src/app/errors_ui.rs
// Purpose: Error reporting UI (dialogs, inline panels, etc.)
// Header-only minimal implementation using Dear ImGui modals.

#include <string>
#include <vector>
#include <utility>

#include "../../vendor/imgui/imgui.h"

namespace app {
namespace errors_ui {

// Simple queue of pending error popups to show sequentially
inline std::vector<std::pair<std::string, std::string>>& pending_errors() {
    static std::vector<std::pair<std::string, std::string>> q;
    return q;
}

// Queue an error to be shown as a modal dialog
inline void push_error(const std::string& title, const std::string& message) {
    pending_errors().emplace_back(title.empty() ? std::string("Error") : title, message);
}

// Render currently queued error (if any). Call once per frame while your frame is active.
inline void render() {
    auto& q = pending_errors();
    if (q.empty()) return;

    // Ensure popup is opened
    const char* popup_id = "##error_popup";
    ImGui::OpenPopup(popup_id);

    // Use title of front error as window title text
    const std::string& title = q.front().first;
    const std::string& message = q.front().second;

    bool open = true;
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::BeginPopupModal(title.c_str(), &open, flags)) {
        ImGui::TextWrapped("%s", message.c_str());
        ImGui::Dummy(ImVec2(1.f, 6.f));
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            // pop current error
            q.erase(q.begin());
        }
        ImGui::EndPopup();
    }

    // If user closed via X, also pop
    if (!open) {
        q.erase(q.begin());
    }
}

// One-shot helper: show an error immediately (push + render)
// Note: You still need to call errors_ui::render() each frame for the modal to appear.
inline void show_error(const std::string& title, const std::string& message) {
    push_error(title, message);
}

} // namespace errors_ui
} // namespace app
