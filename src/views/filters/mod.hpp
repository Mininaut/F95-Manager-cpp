#pragma once
// Port target: src/views/filters/mod.rs
// Purpose: Aggregate filters view module and expose public API.
// In Rust this module re-exports building blocks under views::filters::{items,..} and exposes render entrypoints.

#include "render.hpp"
#include "items/mod.hpp"

namespace views {
namespace filters {

// Facade to render the filters panel (mirrors Rust module-level export)
inline void render_panel(const RenderOptions& opts, Model& model, const tags::Catalog* catalog) {
    render(opts, model, catalog);
}


} // namespace filters
} // namespace views
