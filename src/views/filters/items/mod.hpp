#pragma once
// Port target: src/views/filters/items/* (segmented_panel.rs, mode_switch.rs, discrete_slider.rs,
// search_with_mode.rs, picker.rs, tags_menu.rs, prefixes_menu.rs)
// Purpose: Provide a facade namespace with inline stubs mirroring Rust item APIs so the
// C++ project structure and imports match the Rust project one-to-one.

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdint>

#include "../../../../vendor/imgui/imgui.h"
#include "../../../tags/mod.hpp"

namespace views {
namespace filters {
namespace items {

// Segmented panel (like a segmented control of mutually-exclusive options)
namespace segmented_panel {
inline bool render(const char* id, int& activeIndex, const std::vector<std::string>& segments) {
    if (!id) id = "segmented";
    bool changed = false;
    ImGui::PushID(id);
    for (int i = 0; i < (int)segments.size(); ++i) {
        if (i != 0) ImGui::SameLine();
        bool selected = (i == activeIndex);
        ImVec4 base = selected ? ImVec4(0.18f, 0.45f, 0.90f, 1.0f) : ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(base.x + 0.05f, base.y + 0.05f, base.z + 0.05f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(base.x - 0.05f, base.y - 0.05f, base.z - 0.05f, 1.0f));
        if (ImGui::Button(segments[i].c_str())) {
            if (activeIndex != i) {
                activeIndex = i;
                changed = true;
            }
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::PopID();
    return changed;
}
} // namespace segmented_panel

// Mode switch (two-state switch with labels)
namespace mode_switch {
inline bool render(const char* id, bool& is_on, const char* leftLabel = "Off", const char* rightLabel = "On") {
    if (!id) id = "mode_switch";
    bool changed = false;
    ImGui::PushID(id);
    if (leftLabel) { ImGui::TextUnformatted(leftLabel); ImGui::SameLine(); }
    if (ImGui::Checkbox("##switch", &is_on)) changed = true;
    if (rightLabel) { ImGui::SameLine(); ImGui::TextUnformatted(rightLabel); }
    ImGui::PopID();
    return changed;
}
} // namespace mode_switch

// Discrete slider (integer slider with step)
namespace discrete_slider {
inline bool render(const char* label, int& value, int min_v, int max_v, int step = 1) {
    int old = value;
    if (ImGui::SliderInt(label ? label : "##discrete", &value, min_v, max_v)) {
        if (step > 1) {
            int d = (value - min_v) % step;
            if (d != 0) {
                if (d >= step / 2) value += (step - d);
                else value -= d;
                value = std::min(std::max(value, min_v), max_v);
            }
        }
    }
    return value != old;
}
} // namespace discrete_slider

// Search with mode (text input + segmented modes)
namespace search_with_mode {
inline bool render(const char* id,
                   std::string& text,
                   int& modeIndex,
                   const std::vector<std::string>& modes,
                   const char* placeholder = "Search") {
    if (!id) id = "search_with_mode";
    bool changed = false;
    ImGui::PushID(id);

    // modes segmented panel
    {
        changed |= items::segmented_panel::render("modes", modeIndex, modes);
    }

    // text input (using a local buffer)
    {
        char buf[256];
        size_t n = std::min(text.size(), sizeof(buf) - 1);
        std::memcpy(buf, text.data(), n);
        buf[n] = '\0';
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputTextWithHint("##query", placeholder ? placeholder : "", buf, sizeof(buf))) {
            std::string new_text(buf);
            if (new_text != text) {
                text = std::move(new_text);
                changed = true;
            }
        }
    }

    ImGui::PopID();
    return changed;
}
} // namespace search_with_mode

// Generic picker (e.g., dropdown) — simple combo box placeholder
namespace picker {
inline bool render_combo(const char* label, int& currentIndex, const std::vector<std::string>& options) {
    bool changed = false;
    if (ImGui::BeginCombo(label ? label : "##picker",
                          (currentIndex >= 0 && currentIndex < (int)options.size()) ? options[(size_t)currentIndex].c_str() : "(none)")) {
        for (int i = 0; i < (int)options.size(); ++i) {
            bool selected = (i == currentIndex);
            if (ImGui::Selectable(options[(size_t)i].c_str(), selected)) {
                if (!selected) { currentIndex = i; changed = true; }
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}
} // namespace picker

 // Tags menu — render available tags as chips (read-only; integrate with selection if needed)
namespace tags_menu {
inline void render(const tags::Catalog* cat) {
    if (!cat) return;
    ImGui::TextUnformatted("Tags:");
    int shown = 0;
    for (const auto& kv : cat->tags) {
        if (shown++ % 6 != 0) ImGui::SameLine();
        ImGui::SmallButton(kv.second.c_str());
    }
}

 // Picker returning selected tag id (if any)
inline bool pick(const char* id, const tags::Catalog* cat, std::uint32_t& out_id, const char* placeholder = "(select tag)") {
    if (!cat) return false;

    struct Item { std::uint32_t id; std::string name; };
    std::vector<Item> items;
    items.reserve(cat->tags.size());
    for (const auto& kv : cat->tags) {
        items.push_back(Item{ static_cast<std::uint32_t>(kv.first), kv.second });
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b){ return a.name < b.name; });

    bool chosen = false;
    const char* label = id ? id : "##tags_picker";
    const char* preview = (placeholder && *placeholder) ? placeholder : "(none)";
    if (ImGui::BeginCombo(label, preview)) {
        for (const auto& it : items) {
            const bool selected = false;
            if (ImGui::Selectable(it.name.c_str(), selected)) {
                out_id = it.id;
                chosen = true;
            }
        }
        ImGui::EndCombo();
    }
    return chosen;
}
} // namespace tags_menu

 // Prefixes menu — list groups and prefixes; this is a placeholder UI
namespace prefixes_menu {
inline void render(const tags::Catalog* cat) {
    if (!cat) return;
    ImGui::TextUnformatted("Prefixes:");
    auto render_group = [](const char* name, const std::vector<tags::Group>& groups) {
        if (groups.empty()) return;
        if (ImGui::TreeNode(name)) {
            for (const auto& g : groups) {
                if (ImGui::TreeNode((name + std::string(" - ") + g.name).c_str())) {
                    for (const auto& p : g.prefixes) {
                        ImGui::BulletText("%s", p.name.c_str());
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
    };
    render_group("Games", cat->games);
    render_group("Comics", cat->comics);
    render_group("Animations", cat->animations);
    render_group("Assets", cat->assets);
}

 // Picker returning selected prefix id (if any)
inline bool pick(const char* id, const tags::Catalog* cat, std::uint32_t& out_id, const char* placeholder = "(select prefix)") {
    if (!cat) return false;

    struct Item { std::uint32_t id; std::string name; };
    std::vector<Item> items;
    auto emit = [&](const char* cat_name, const std::vector<tags::Group>& groups) {
        for (const auto& g : groups) {
            for (const auto& p : g.prefixes) {
                items.push_back(Item{ static_cast<std::uint32_t>(p.id), std::string(cat_name) + ": " + p.name });
            }
        }
    };
    emit("Games", cat->games);
    emit("Comics", cat->comics);
    emit("Animations", cat->animations);
    emit("Assets", cat->assets);
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b){ return a.name < b.name; });

    bool chosen = false;
    const char* label = id ? id : "##prefixes_picker";
    const char* preview = (placeholder && *placeholder) ? placeholder : "(none)";
    if (ImGui::BeginCombo(label, preview)) {
        for (const auto& it : items) {
            const bool selected = false;
            if (ImGui::Selectable(it.name.c_str(), selected)) {
                out_id = it.id;
                chosen = true;
            }
        }
        ImGui::EndCombo();
    }
    return chosen;
}
} // namespace prefixes_menu

} // namespace items
} // namespace filters
} // namespace views
