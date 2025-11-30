#pragma once
// Port target: src/views/* (module aggregator similar to Rust views/mod.rs)
// Purpose: Provide a simple facade to the cards and filters modules so other parts
// of the app can import `views::*` in a way that mirrors the Rust project.

#include "cards/mod.hpp"
#include "filters/mod.hpp"

namespace views {

// Optional shared render context placeholder for future expansion
struct RenderContext {};

// Re-export-like thin facades to keep call sites close to Rust style

// Filters
using FiltersRenderOptions = ::views::filters::RenderOptions;
using FiltersModel = ::views::filters::Model;
using FiltersEvents = ::views::filters::Events;

inline void render_filters_panel(const FiltersRenderOptions& opts,
                                 FiltersModel& model,
                                 const ::tags::Catalog* catalog) {
    ::views::filters::render_panel(opts, model, catalog);
}

// Rust-like facade returning event flags (changed/settings/logs/about)
inline FiltersEvents draw_filters_panel(const FiltersRenderOptions& opts,
                                        FiltersModel& model,
                                        const ::tags::Catalog* catalog) {
    return ::views::filters::draw_filters_panel(opts, model, catalog);
}

// Cards (facade to thread_card/draw_cards_grid)
inline void render_thread_card(const ::parser::GameInfo& gi,
                               float width,
                               const ::app::settings::Config* cfg,
                               const ::tags::Catalog* cat) {
    ::views::cards::thread_card(gi, width, cfg, cat);
}

inline void render_cards_grid(const std::vector<::parser::GameInfo>& items,
                              float cardWidth,
                              const ::app::settings::Config* cfg,
                              const ::tags::Catalog* cat,
                              float spacing) {
    ::views::cards::draw_cards_grid(items, cardWidth, cfg, cat, spacing);
}

} // namespace views
