#pragma once
// Port target: src/parser/game_info/link/gofile.rs
// Purpose: Mirror Rust submodule structure; forward to generic link::parse with gofile hint.

#include <string>
#include "mod.hpp"

namespace parser {
namespace game_info {
namespace link {
namespace gofile {

inline LinkInfo parse(const std::string& url) {
    LinkInfo li = link::parse(url);
    // Mark provider intent as 'gofile' to match Rust structure semantics.
    li.provider = "gofile";
    return li;
}

} // namespace gofile
} // namespace link
} // namespace game_info
} // namespace parser
