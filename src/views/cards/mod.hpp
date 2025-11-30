#pragma once
// Port target: src/views/cards/mod.rs
// Purpose: Aggregate cards view module and expose public API.
// In Rust this module re-exports thread_card from items/card.rs.
// Here we provide a thin facade that maps to render.hpp.

#include "render.hpp"
#include "items/card.hpp"
#include "items/cover_helpers.hpp"

namespace views {
namespace cards {

// Re-export-like facade to mirror Rust's `pub use card::thread_card;`
inline void thread_card(const parser::GameInfo& gi,
                        float width,
                        const app::settings::Config* cfg,
                        const tags::Catalog* cat) {
    draw_thread_card(gi, width, cfg, cat);
}

} // namespace cards
} // namespace views
