#pragma once
// Port target: src/parser/game_info/link/archive.rs
// Purpose: Mirror Rust submodule structure; forward to generic link::parse with archive hint.

#include <string>
#include "mod.hpp"

namespace parser {
namespace game_info {
namespace link {
namespace archive {

inline LinkInfo parse(const std::string& url) {
    LinkInfo li = link::parse(url);
    // In this facade, mark provider as 'archive' to mirror Rust submodule intent.
    li.provider = "archive";
    return li;
}

} // namespace archive
} // namespace link
} // namespace game_info
} // namespace parser
