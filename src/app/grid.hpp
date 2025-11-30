#pragma once
// Port target: src/app/grid.rs
// Purpose: Grid layout/controller for cards (scrolling, selection, etc.)

namespace app {
namespace grid {

// Compute simple grid layout columns count given available content width, fixed card width and spacing.
inline void layout(float content_width, float card_width, float spacing, int& out_columns) {
    if (card_width <= 0.0f) { out_columns = 1; return; }
    float denom = (card_width + spacing);
    if (denom <= 0.0f) { out_columns = 1; return; }
    int cols = static_cast<int>((content_width + spacing) / denom);
    if (cols < 1) cols = 1;
    out_columns = cols;
}

// Scroll to a given item index (no-op in this lightweight port; left for integration with a scroller).
inline void scroll_to(int /*index*/) {
    // no-op
}

} // namespace grid
} // namespace app
