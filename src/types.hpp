#pragma once
// Port target: src/types.rs
// Shared basic types and enums used across modules to mirror Rust UI logic.

#include <string>
#include <vector>
#include <cstdint>

namespace types {

// Common aliases
using Bytes = std::vector<std::uint8_t>;

struct Error {
    std::string message;
    // optional: error code, category, etc.
};

// UI enums (mirror Rust)
enum class Sorting {
    Date,
    Likes,
    Views,
    Title,
    Rating
};

enum class DateLimit {
    Anytime,
    Today,
    Days3,
    Days7,
    Days14,
    Days30,
    Days90,
    Days180,
    Days365
};

enum class TagLogic {
    Or,
    And
};

enum class SearchMode {
    Creator,
    Title
};

} // namespace types
