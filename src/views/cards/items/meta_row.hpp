#pragma once
// Port target: src/views/cards/items/meta_row.rs
// Purpose: Draw the meta row for a thread card (version, counters, etc.)

#include <string>
#include <vector>

#include "../../../../vendor/imgui/imgui.h"
#include "../../../parser/parser.hpp"

namespace views {
namespace cards {
namespace items {
namespace meta_row {

inline void render(const parser::GameInfo& gi, float /*inner_w*/) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.85f, 1.0f));
    ImGui::TextUnformatted(gi.meta.version.empty() ? "v?" : gi.meta.version.c_str());
    ImGui::PopStyleColor();
}

} // namespace meta_row
} // namespace items
} // namespace cards
} // namespace views
