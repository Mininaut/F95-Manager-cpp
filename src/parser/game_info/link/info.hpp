#pragma once
// Port target: src/parser/game_info/link/info.rs
// Purpose: Mirror Rust submodule structure; forward to generic link::parse with info hint.

#include <string>
#include "mod.hpp"

namespace parser {
namespace game_info {
namespace link {
namespace info {

inline LinkInfo parse(const std::string& url) {
    LinkInfo li = link::parse(url);
    // Mark provider intent as 'info' to match Rust structure semantics.
    li.provider = "info";
    return li;
}

} // namespace info
} // namespace link
} // namespace game_info
} // namespace parser
