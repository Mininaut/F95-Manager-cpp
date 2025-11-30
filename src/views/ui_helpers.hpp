#pragma once
// Port target: src/views/ui_helpers.rs
// Purpose: Common UI helper utilities (layout, formatting, resource helpers, etc.)

#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <functional>
#include "../../vendor/imgui/imgui.h"

namespace views {
namespace ui_helpers {

// Clamp utility (templated)
template <typename T>
inline T clamp(T v, T lo, T hi) {
    return std::min(std::max(v, lo), hi);
}

// Format large integer counts into compact form similar to Rust formatter:
//  - 0..999 -> "123"
//  - 1_000..999_999 -> "12.3K"
//  - 1_000_000.. -> "3.4M"
inline std::string format_count(std::uint64_t value) {
    std::ostringstream os;
    if (value < 1000ULL) {
        os << value;
        return os.str();
    }
    if (value < 1'000'000ULL) {
        double k = static_cast<double>(value) / 1000.0;
        os << std::fixed << std::setprecision(k >= 100.0 ? 0 : 1) << k << "K";
        return os.str();
    }
    double m = static_cast<double>(value) / 1'000'000.0;
    os << std::fixed << std::setprecision(m >= 100.0 ? 0 : 1) << m << "M";
    return os.str();
}

/**
 * Sticky overlay helper (approximation of Rust's show_sticky_overlay).
 * Shows a small auto-sized popup above/below the given rectangle while mouse hovers the rect.
 * Usage:
 *   views::ui_helpers::show_sticky_overlay(rect_min, rect_max, "warn_overlay", 6.f, 4.f, []{
 *       ImGui::TextUnformatted("Hello");
 *   });
 */
inline void show_sticky_overlay(const ImVec2& rect_min,
                               const ImVec2& rect_max,
                               const char* id_ns,
                               float margin_v,
                               float margin_h,
                               const std::function<void()>& draw_content) {
    // Hover detection over anchor rect
    ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool hovered = (mouse.x >= rect_min.x && mouse.x <= rect_max.x &&
                          mouse.y >= rect_min.y && mouse.y <= rect_max.y);
    if (!hovered) return;

    // Compute default position above the rect; if not enough space, place below
    ImVec2 size_hint(220.f, 0.f); // will auto resize, this is just for positioning
    ImVec2 pos(rect_min.x, rect_min.y - margin_v);
    const float screen_top = ImGui::GetMainViewport()->WorkPos.y;
    if (pos.y - size_hint.y < screen_top) {
        pos.y = rect_max.y + margin_v; // place below if not enough space above
    }
    pos.x = rect_min.x + margin_h;

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.97f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoSavedSettings
                           | ImGuiWindowFlags_NoFocusOnAppearing
                           | ImGuiWindowFlags_NoNav;

    std::string win_id = "##overlay_";
    win_id += (id_ns ? id_ns : "overlay");

    if (ImGui::Begin(win_id.c_str(), nullptr, flags)) {
        if (draw_content) {
            draw_content();
        }
    }
    ImGui::End();
}

} // namespace ui_helpers
} // namespace views
