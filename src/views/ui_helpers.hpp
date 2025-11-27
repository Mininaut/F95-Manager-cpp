#pragma once
// Port target: src/views/ui_helpers.rs
// Purpose: Common UI helper utilities (layout, formatting, resource helpers, etc.)

#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>

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

} // namespace ui_helpers
} // namespace views
