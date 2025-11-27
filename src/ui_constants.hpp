#pragma once
// Port target: src/ui_constants.rs
// Centralized UI constants (sizes, paddings, colors, timings, etc.)

#include <cstdint>

namespace ui_constants {

// Example placeholders to mirror Rust constants; adjust during port.
inline constexpr int kCardWidth = 280;
inline constexpr int kCardHeight = 400;
inline constexpr int kPadding = 8;
inline constexpr int kSpacing = 6;
inline constexpr int kMaxFilterItems = 10;

// Timing (ms)
inline constexpr std::uint32_t kHoverDelayMs = 150;

} // namespace ui_constants
