#pragma once
// Port target: src/parser/game_info/link/direct.rs
// Purpose: Mirror Rust submodule structure; forward to generic link::parse with direct hint.

#include <string>
#include "mod.hpp"

namespace parser {
namespace game_info {
namespace link {
namespace direct {

inline LinkInfo parse(const std::string& url) {
    LinkInfo li = link::parse(url);
    // In this facade, mark provider as 'direct' to mirror Rust submodule intent.
    li.provider = "direct";
    return li;
}

} // namespace direct
} // namespace link
} // namespace game_info
} // namespace parser
