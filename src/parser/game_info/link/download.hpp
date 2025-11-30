#pragma once
// Port target: src/parser/game_info/link/download.rs
// Purpose: Mirror Rust submodule structure; forward to generic link::parse with download hint.

#include <string>
#include "mod.hpp"

namespace parser {
namespace game_info {
namespace link {
namespace download {

inline LinkInfo parse(const std::string& url) {
    LinkInfo li = link::parse(url);
    // Mark provider/type intent as 'download' to match Rust structure semantics.
    li.provider = "download";
    return li;
}

} // namespace download
} // namespace link
} // namespace game_info
} // namespace parser
