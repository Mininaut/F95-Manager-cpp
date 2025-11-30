#pragma once
// Port target: src/parser/game_info/page.rs
// Purpose: Provide a page type to mirror Rust structure.
// In this C++ port, F95Page is defined in types.hpp. This header re-exports it
// so includes can mirror the Rust module tree.

#include "types.hpp"

namespace parser {
namespace game_info {

// Alias to keep includes semantically similar to Rust's page module
using Page = F95Page;

} // namespace game_info
} // namespace parser
