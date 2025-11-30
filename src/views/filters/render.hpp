#pragma once
// Port target: src/views/filters/render.rs
// Purpose: Rendering logic for filters panel (sorting, date limit, search, tags/prefixes).

#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>

#include "../../../vendor/imgui/imgui.h"
#include "../ui_helpers.hpp"
#include "../cards/items/tags_panel.hpp"
#include "../../types.hpp"
#include "../../ui_constants.hpp"
#include "../filters/items/mod.hpp"
#include "../../tags/mod.hpp"

namespace views {
namespace filters {

struct RenderOptions {
    bool show_library_toggle = true;
    bool show_search = true;
    bool show_sorting = true;
    bool show_date_limit = true;
    bool show_tag_menus = true;
};

// Simple model to hold filter values (mirror of Rust-side state but simplified)
struct Model {
    bool library_only = false;

    // basic filters
    int sorting_index = 0;     // maps to types::Sorting
    int date_limit_index = 0;  // maps to types::DateLimit
    int search_mode_index = 0; // maps to types::SearchMode
    std::string query;

    // advanced filters
    int include_logic_index = 0; // 0 = OR, 1 = AND
    std::vector<std::uint32_t> include_tags;
    std::vector<std::uint32_t> exclude_tags;
    std::vector<std::uint32_t> include_prefixes;
    std::vector<std::uint32_t> exclude_prefixes;
};

 // Events returned from filters panel rendering
struct Events {
    bool changed = false;
    bool settings_clicked = false;
    bool logs_clicked = false;
    bool about_clicked = false;
};

// Render removable items utility (tags/prefixes with close '×' button)
template <typename Resolver>
inline bool render_removable_items(std::vector<std::uint32_t>& items, Resolver resolver) {
    bool removed = false;
    int remove_index = -1;
    int shown = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        if (shown++ % 6 != 0) ImGui::SameLine();
        std::string name = resolver(static_cast<std::uint32_t>(items[i]));
        std::string label = name.empty() ? ("#" + std::to_string(items[i]) + " ×") : (name + " ×");
        if (ImGui::SmallButton(label.c_str())) {
            remove_index = static_cast<int>(i);
        }
    }
    if (remove_index >= 0) {
        items.erase(items.begin() + remove_index);
        removed = true;
    }
    return removed;
}

// Draws the right-side filters panel, mutates model and returns event flags.
inline Events draw_filters_panel(const RenderOptions& opts, Model& model, const tags::Catalog* catalog) {
    Events ev{};

    using namespace ::views::filters::items;

    if (opts.show_library_toggle) {
        ImGui::SeparatorText("Scope");
        bool on = model.library_only;
        if (mode_switch::render("library_toggle", on, "All", "Library")) {
            model.library_only = on;
            ev.changed = true;
        }
        ImGui::Dummy(ImVec2(1, ui_constants::kSpacing));
    }

    if (opts.show_sorting) {
        ImGui::SeparatorText("Sorting");
        static const std::vector<std::string> segs = { "Date", "Likes", "Views", "Title", "Rating" };
        int idx = model.sorting_index;
        if (segmented_panel::render("sorting", idx, segs)) {
            model.sorting_index = idx;
            ev.changed = true;
        }
        ImGui::Dummy(ImVec2(1, ui_constants::kSpacing));
    }

    if (opts.show_date_limit) {
        ImGui::SeparatorText("Date Limit");
        static const std::vector<std::string> options = {
            "Anytime", "Today", "3 days", "7 days", "14 days", "30 days", "90 days", "180 days", "365 days"
        };
        int c = model.date_limit_index;
        picker::render_combo("Preset", c, options);
        ImGui::SameLine();
        int v = c;
        if (discrete_slider::render("##date_limit", v, 0, (int)options.size() - 1, 1)) {
            c = v;
        }
        if (c != model.date_limit_index) {
            model.date_limit_index = c;
            ev.changed = true;
        }
        ImGui::Dummy(ImVec2(1, ui_constants::kSpacing));
    }

    if (opts.show_search) {
        ImGui::SeparatorText("Search");
        static const std::vector<std::string> modes = { "Creator", "Title" };
        int m = model.search_mode_index;
        bool changed = search_with_mode::render("search", model.query, m, modes, "Search...");
        if (m != model.search_mode_index) { model.search_mode_index = m; ev.changed = true; }
        if (changed) ev.changed = true;
        ImGui::Dummy(ImVec2(1, ui_constants::kSpacing));
    }

    if (opts.show_tag_menus && catalog) {
        ImGui::SeparatorText("Tags (include)");
        {
            bool is_and = (model.include_logic_index == 1);
            if (mode_switch::render("include_logic", is_and, "OR", "AND")) {
                model.include_logic_index = is_and ? 1 : 0;
                ev.changed = true;
            }
        }
        {
            std::uint32_t picked = 0;
            if (tags_menu::pick("include_tags", catalog, picked, "Select tag to include")) {
                auto& v = model.include_tags;
                if ((int)v.size() < ui_constants::kMaxFilterItems &&
                    std::find(v.begin(), v.end(), picked) == v.end()) {
                    v.push_back(picked);
                    model.query.clear();
                    ev.changed = true;
                }
            }
            if (render_removable_items(model.include_tags, [&](std::uint32_t id){ return tags::tag_name_by_id(*catalog, id); })) {
                ev.changed = true;
            }
        }

        ImGui::SeparatorText("Tags (exclude)");
        {
            std::uint32_t picked = 0;
            if (tags_menu::pick("exclude_tags", catalog, picked, "Select tag to exclude")) {
                auto& v = model.exclude_tags;
                if ((int)v.size() < ui_constants::kMaxFilterItems &&
                    std::find(v.begin(), v.end(), picked) == v.end()) {
                    v.push_back(picked);
                    model.query.clear();
                    ev.changed = true;
                }
            }
            if (render_removable_items(model.exclude_tags, [&](std::uint32_t id){ return tags::tag_name_by_id(*catalog, id); })) {
                ev.changed = true;
            }
        }

        ImGui::SeparatorText("Prefixes (include)");
        {
            std::uint32_t picked = 0;
            if (prefixes_menu::pick("include_prefixes", catalog, picked, "Select prefix to include")) {
                auto& v = model.include_prefixes;
                if ((int)v.size() < ui_constants::kMaxFilterItems &&
                    std::find(v.begin(), v.end(), picked) == v.end()) {
                    v.push_back(picked);
                    ev.changed = true;
                }
            }
            if (render_removable_items(model.include_prefixes, [&](std::uint32_t id){ return tags::prefix_name_by_id(*catalog, id); })) {
                ev.changed = true;
            }
        }

        ImGui::SeparatorText("Prefixes (exclude)");
        {
            std::uint32_t picked = 0;
            if (prefixes_menu::pick("exclude_prefixes", catalog, picked, "Select prefix to exclude")) {
                auto& v = model.exclude_prefixes;
                if ((int)v.size() < ui_constants::kMaxFilterItems &&
                    std::find(v.begin(), v.end(), picked) == v.end()) {
                    v.push_back(picked);
                    ev.changed = true;
                }
            }
            if (render_removable_items(model.exclude_prefixes, [&](std::uint32_t id){ return tags::prefix_name_by_id(*catalog, id); })) {
                ev.changed = true;
            }
        }

        ImGui::Dummy(ImVec2(1, ui_constants::kSpacing));
    }

    // Bottom actions
    if (ImGui::Button("Logs")) { ev.logs_clicked = true; }
    if (ImGui::Button("About")) { ev.about_clicked = true; }
    if (ImGui::Button("Settings")) { ev.settings_clicked = true; }

    return ev;
}

// Render the filters panel. Mutates the provided model in-place.
inline void render(const RenderOptions& opts, Model& model, const tags::Catalog* catalog) {
    using namespace ::views::filters::items;

    if (opts.show_library_toggle) {
        ImGui::SeparatorText("Scope");
        bool on = model.library_only;
        if (mode_switch::render("library_toggle", on, "All", "Library")) {
            model.library_only = on;
        }
        ImGui::Dummy(ImVec2(1, 6));
    }

    if (opts.show_search) {
        ImGui::SeparatorText("Search");
        static const std::vector<std::string> modes = { "Creator", "Title" };
        search_with_mode::render("search", model.query, model.search_mode_index, modes, "Search...");
        ImGui::Dummy(ImVec2(1, 6));
    }

    if (opts.show_sorting) {
        ImGui::SeparatorText("Sorting");
        static const std::vector<std::string> segs = { "Date", "Likes", "Views", "Title", "Rating" };
        segmented_panel::render("sorting", model.sorting_index, segs);
        ImGui::Dummy(ImVec2(1, 6));
    }

    if (opts.show_date_limit) {
        ImGui::SeparatorText("Date Limit");
        // Discrete slider over predefined stops 0..8 (Any..365d)
        // For visual clarity, also provide a combo box
        static const std::vector<std::string> options = {
            "Anytime", "Today", "3 days", "7 days", "14 days", "30 days", "90 days", "180 days", "365 days"
        };
        picker::render_combo("Preset", model.date_limit_index, options);
        ImGui::SameLine();
        int v = model.date_limit_index;
        if (discrete_slider::render("##date_limit", v, 0, (int)options.size() - 1, 1)) {
            model.date_limit_index = v;
        }
        ImGui::Dummy(ImVec2(1, 6));
    }

    if (opts.show_tag_menus && catalog) {
        ImGui::SeparatorText("Tags & Prefixes");
        ImGui::BeginChild("tags_prefixes", ImVec2(0, 150), true);
        // Left: Tags menu
        ImGui::BeginGroup();
        tags_menu::render(catalog);
        ImGui::EndGroup();

        ImGui::SameLine();

        // Right: Prefixes tree
        ImGui::BeginGroup();
        prefixes_menu::render(catalog);
        ImGui::EndGroup();

        ImGui::EndChild();
    }
}

} // namespace filters
} // namespace views
