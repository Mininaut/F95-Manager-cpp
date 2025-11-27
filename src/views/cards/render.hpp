#pragma once
// Port target: src/views/cards/render.rs (thread_card facade)
// Purpose: ImGui-based card rendering to visually mirror original Rust UI.

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#include "../../../vendor/imgui/imgui.h"
#include "../../parser/parser.hpp"
#include "../../ui_constants.hpp"
#include "../ui_helpers.hpp"
#include "items/cover_helpers.hpp"
#include "../../app/settings/settings.hpp"
#include "../../tags/mod.hpp"
#include "../../app/settings/helpers/open.hpp"

namespace views {
namespace cards {

// Small "chip" button for tags/prefixes
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

// Draw a cover placeholder with 16:9 aspect to match original tile visual weight
inline void draw_cover_placeholder(float width) {
    const float h = width * 9.0f / 16.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 rectMax = ImVec2(pos.x + width, pos.y + h);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, rectMax, IM_COL32(58, 58, 58, 255), 6.0f);
    // subtle border
    dl->AddRect(pos, rectMax, IM_COL32(84, 84, 84, 255), 6.0f, 0, 2.0f);
    ImGui::Dummy(ImVec2(width, h));
}

inline void draw_cover(const parser::GameInfo& gi, float width, const app::settings::Config* cfg, const tags::Catalog* cat) {
    const float h = width * 9.0f / 16.0f;
    // Make the cover area an interactive item for hover/click handling
    ImGui::PushID(gi.meta.title.c_str());
    ImGui::InvisibleButton("cover", ImVec2(width, h));
    ImVec2 pos = ImGui::GetItemRectMin();
    ImVec2 rectMax = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Cover placeholder
    const float rounding = 6.0f;
    dl->AddRectFilled(pos, rectMax, IM_COL32(58, 58, 58, 255), rounding);
    dl->AddRect(pos, rectMax, IM_COL32(84, 84, 84, 255), rounding, 0, 2.0f);

    // Version badge (top-right)
    std::string ver = gi.meta.version.empty() ? "v?" : gi.meta.version;
    ImVec2 txt = ImGui::CalcTextSize(ver.c_str());
    const float pad_x = 6.0f, pad_y = 4.0f;
    const float badge_w = txt.x + 2.0f * pad_x;
    const float badge_h = txt.y + 2.0f * pad_y;
    ImVec2 badgeMin(rectMax.x - (float)ui_constants::kPadding - badge_w, pos.y + (float)ui_constants::kPadding);
    ImVec2 badgeMax(badgeMin.x + badge_w, badgeMin.y + badge_h);
    dl->AddRectFilled(badgeMin, badgeMax, IM_COL32(32, 120, 200, 230), 6.0f);
    dl->AddText(ImVec2(badgeMin.x + pad_x, badgeMin.y + pad_y), IM_COL32(255, 255, 255, 255), ver.c_str());

    // Engine badge (top-left) if resolvable from prefixes or title
    if (cat) {
        std::string engine = views::cards::items::cover_helpers::resolve_engine_name(gi, *cat);
        if (!engine.empty()) {
            ImVec2 etxt = ImGui::CalcTextSize(engine.c_str());
            const float epad_x = 6.0f, epad_y = 4.0f;
            const float ebadge_w = etxt.x + 2.0f * epad_x;
            const float ebadge_h = etxt.y + 2.0f * epad_y;
            ImVec2 emin(pos.x + (float)ui_constants::kPadding, pos.y + (float)ui_constants::kPadding);
            ImVec2 emax(emin.x + ebadge_w, emin.y + ebadge_h);
            dl->AddRectFilled(emin, emax, IM_COL32(50, 170, 110, 230), 6.0f);
            dl->AddText(ImVec2(emin.x + epad_x, emin.y + epad_y), IM_COL32(255, 255, 255, 255), engine.c_str());
        }
    }

    // Hover markers area at the very bottom of the cover (inside the image), similar to Rust
    const float markers_h = 12.0f;
    ImVec2 markersMin = ImVec2(pos.x, rectMax.y - markers_h);
    ImVec2 markersMax = ImVec2(rectMax.x, rectMax.y);
    // Determine number of segments (use links count if available, min 3, max 10)
    int segments = (int)gi.links.size();
    segments = segments <= 0 ? 5 : segments;
    segments = segments > 10 ? 10 : segments;
    // Hover detection
    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool over_cover = ImGui::IsItemHovered();
    bool over_markers = (mouse.x >= markersMin.x && mouse.x <= markersMax.x && mouse.y >= markersMin.y && mouse.y <= markersMax.y);
    int hovered_seg = -1;
    if (over_cover || over_markers) {
        float relx = views::ui_helpers::clamp(mouse.x - pos.x, 0.0f, width);
        float seg_w = width / (float)segments;
        hovered_seg = (int)std::floor(relx / seg_w);
        if (hovered_seg < 0) hovered_seg = 0;
        if (hovered_seg >= segments) hovered_seg = segments - 1;
    }
    // If clicked on cover, open hovered segment link
    if ((over_cover || over_markers) && ImGui::IsItemClicked() && !gi.links.empty()) {
        int idx = (int)gi.links.size() - 1;
        if (hovered_seg >= 0) idx = std::min(hovered_seg, (int)gi.links.size() - 1);
        app::settings::helpers::open::url(gi.links[(size_t)idx].url);
    }

    // Draw segments
    if (segments > 0) {
        float seg_w = width / (float)segments;
        for (int i = 0; i < segments; ++i) {
            float x0 = pos.x + seg_w * (float)i + 1.0f;
            float x1 = pos.x + seg_w * (float)(i + 1) - 1.0f;
            ImU32 col = (i == hovered_seg) ? IM_COL32(80, 160, 255, 220) : IM_COL32(200, 200, 200, 80);
            dl->AddRectFilled(ImVec2(x0, markersMin.y), ImVec2(x1, markersMax.y), col, 2.0f);
        }
    }

    // Thin progress line along the bottom of the cover (indeterminate if no specific progress)
    {
        double t = ImGui::GetTime(); // seconds
        float dp = fmodf((float)(t * 0.35), 1.0f); // slow moving
        float x0 = pos.x;
        float x1 = x0 + width * dp;
        float y1 = rectMax.y;
        float y0 = y1 - 2.0f;
        ImU32 color = IM_COL32(60, 140, 250, 220);
        dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color, 0.0f);
    }

    // Optional warning count badge (bottom-left) — real logic if cfg/catalog provided, else heuristic
    int warn_count = 0;
    if (cfg && cat) {
        auto wp = views::cards::items::cover_helpers::collect_warnings(gi, *cfg, *cat);
        warn_count = static_cast<int>(wp.first.size() + wp.second.size());
    } else {
        warn_count = (int)gi.meta.tags.size() > 8 ? 1 : 0;
    }
    if (warn_count > 0) {
        std::string warn = "!" + std::to_string(warn_count);
        ImVec2 wtxt = ImGui::CalcTextSize(warn.c_str());
        const float wpad_x = 5.0f, wpad_y = 3.0f;
        ImVec2 wMin = ImVec2(pos.x + (float)ui_constants::kPadding, rectMax.y - (float)ui_constants::kPadding - (wtxt.y + 2.0f * wpad_y));
        ImVec2 wMax = ImVec2(wMin.x + wtxt.x + 2.0f * wpad_x, wMin.y + wtxt.y + 2.0f * wpad_y);
        dl->AddRectFilled(wMin, wMax, IM_COL32(255, 196, 0, 230), 6.0f);
        dl->AddText(ImVec2(wMin.x + wpad_x, wMin.y + wpad_y), IM_COL32(20, 20, 20, 255), warn.c_str());
    }

    // Reserve layout space for the cover: InvisibleButton already consumed it
    ImGui::PopID();
}

// Meta row: version on left, basic counters on right if available
inline void draw_meta_row(const parser::GameInfo& gi, float inner_w) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.85f, 1.0f));
    ImGui::TextUnformatted(gi.meta.version.empty() ? "v?" : gi.meta.version.c_str());
    ImGui::PopStyleColor();
    (void)inner_w;
}

// Render a single thread card resembling original layout.
// width: fixed card width (use ui_constants::kCardWidth).
inline void draw_thread_card(const parser::GameInfo& gi, float width, const app::settings::Config* cfg, const tags::Catalog* cat) {
    const float pad = (float)ui_constants::kPadding;
    const float spacing = (float)ui_constants::kSpacing;
    const float inner_w = width - pad * 2.0f;

    // Card frame as a child to keep fixed width
    ImGui::BeginGroup();
    {
        // Outer frame
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad, pad));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
        ImGui::BeginChild(("card##" + gi.meta.title).c_str(), ImVec2(width, 0), true, ImGuiWindowFlags_None);

        // Cover at top
        draw_cover(gi, inner_w, cfg, cat);

        // Title (wrapped)
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + inner_w);
        ImGui::TextUnformatted(gi.meta.title.empty() ? "(no title)" : gi.meta.title.c_str());
        ImGui::PopTextWrapPos();

        // Creator (author) under title
        if (!gi.meta.author.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.90f, 1.0f));
            ImGui::Text("%s", gi.meta.author.c_str());
            ImGui::PopStyleColor();
        }

        // Fixed gap like original
        ImGui::Dummy(ImVec2(1, 6));

        // Meta row (version etc.)
        draw_meta_row(gi, inner_w);

        // Tags as chips
        if (!gi.meta.tags.empty()) {
            ImGui::Dummy(ImVec2(1, 2));
            // Wrap chips
            float x0 = ImGui::GetCursorPosX();
            float x = x0;
            float avail = inner_w;
            for (size_t i = 0; i < gi.meta.tags.size(); ++i) {
                const std::string& t = gi.meta.tags[i];
                ImVec2 label_size = ImGui::CalcTextSize(t.c_str());
                float chip_w = label_size.x + 16.0f; // padding inside chip()
                if ((x - x0) + chip_w > avail) {
                    // new line
                    ImGui::NewLine();
                    x = x0;
                } else if (i != 0) {
                    ImGui::SameLine();
                }
                chip(t);
                x += chip_w + ImGui::GetStyle().ItemSpacing.x;
            }
        }

        // Links (provider + open)
        if (!gi.links.empty()) {
            ImGui::Dummy(ImVec2(1, 6));
            ImGui::Separator();
            ImGui::TextUnformatted("Links:");
            int idx = 0;
            for (const auto& li : gi.links) {
                ImGui::Text("%d. [%s] (%s)", ++idx, li.provider.c_str(), li.type.c_str());
                ImGui::SameLine();
#if defined(_WIN32)
                if (ImGui::SmallButton(("Open##" + std::to_string(idx)).c_str())) {
                    app::settings::helpers::open::url(li.url);
                }
#endif
                ImGui::SameLine();
                ImGui::TextUnformatted(li.url.c_str());
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
    }
    ImGui::EndGroup();
}

// Render a simple grid of cards within current content region.
// The data source can be a vector of GameInfo if multiple items are present.
inline void draw_cards_grid(const std::vector<parser::GameInfo>& items, float cardWidth, const app::settings::Config* cfg, const tags::Catalog* cat, float spacing = (float)ui_constants::kSpacing) {
    float avail = ImGui::GetContentRegionAvail().x;
    int cols = (int)std::max(1.0f, std::floor((avail + spacing) / (cardWidth + spacing)));
    int col = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        draw_thread_card(items[i], cardWidth, cfg, cat);
        col++;
        if (col < cols) {
            ImGui::SameLine(0.0f, spacing);
        } else {
            col = 0;
        }
    }
}

} // namespace cards
} // namespace views
