#pragma once
// Port target: src/views/cards/items/cover_hover.rs
// Purpose: Helpers for hover/click markers over the card cover.

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../../../vendor/imgui/imgui.h"

namespace views {
namespace cards {
namespace items {
namespace cover_hover {

// Compute which segment index is hovered based on mouse position.
// segments: number of markers (>=1)
// coverMin/coverMax: rectangle of the cover area in screen coordinates
inline int segment_hover_index(float width, int segments, ImVec2 coverMin, ImVec2 mousePos) {
    if (segments <= 0 || width <= 1.f) return -1;
    if (mousePos.x < coverMin.x || mousePos.x > coverMin.x + width) return -1;
    float relx = mousePos.x - coverMin.x;
    if (relx < 0.0f) relx = 0.0f;
    if (relx > width) relx = width;
    float seg_w = width / (float)segments;
    int idx = (int)std::floor(relx / seg_w);
    if (idx < 0) idx = 0;
    if (idx >= segments) idx = segments - 1;
    return idx;
}

// Render bottom markers strip inside [min,max] rect.
// hovered: index of hovered segment or -1.
inline void render_markers(ImDrawList* dl, ImVec2 min, ImVec2 max, int segments, int hovered) {
    if (!dl || segments <= 0) return;
    float width = max.x - min.x;
    float seg_w = width / (float)segments;
    for (int i = 0; i < segments; ++i) {
        float x0 = min.x + seg_w * (float)i + 1.0f;
        float x1 = min.x + seg_w * (float)(i + 1) - 1.0f;
        ImU32 col = (i == hovered) ? IM_COL32(80, 160, 255, 220) : IM_COL32(200, 200, 200, 80);
        dl->AddRectFilled(ImVec2(x0, min.y), ImVec2(x1, max.y), col, 2.0f);
    }
}

} // namespace cover_hover
} // namespace items
} // namespace cards
} // namespace views
