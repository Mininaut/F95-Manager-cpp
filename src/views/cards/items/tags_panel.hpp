#pragma once
// Port target: src/views/cards/items/tags_panel.rs
// Purpose: Tag chip rendering and wrapping for thread cards.

#include <string>
#include <vector>
#include <algorithm>

#include "../../../../vendor/imgui/imgui.h"
#include "../../ui_helpers.hpp"

namespace views {
namespace cards {
namespace items {
namespace tags_panel {

// Small "chip" button for tags/prefixes (moved from render.hpp to mirror Rust structure)
inline void chip(const std::string& text) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 4.f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.90f, 1.0f));
    ImGui::Button(text.c_str());
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
}

// Render a list of tags as chips with simple wrapping within inner_w width.
inline void render_chips(const std::vector<std::string>& tags, float inner_w) {
    if (tags.empty()) return;

    ImGui::Dummy(ImVec2(1, 2));
    float x0 = ImGui::GetCursorPosX();
    float x = x0;
    float avail = inner_w;

    for (size_t i = 0; i < tags.size(); ++i) {
        const std::string& t = tags[i];
        ImVec2 label_size = ImGui::CalcTextSize(t.c_str());
        float chip_w = label_size.x + 16.0f; // padding inside chip()

        if ((x - x0) + chip_w > avail) {
            ImGui::NewLine();
            x = x0;
        } else if (i != 0) {
            ImGui::SameLine();
        }

        chip(t);
        x += chip_w + ImGui::GetStyle().ItemSpacing.x;
    }
}

} // namespace tags_panel
} // namespace items
} // namespace cards
} // namespace views
