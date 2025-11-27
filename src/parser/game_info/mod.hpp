#pragma once
// Port target: src/parser/game_info.rs
// Aggregates game thread/page parsing, metadata, and link extraction.
// This C++ port reuses the lightweight regex-based parser in ../parser.hpp
// and exposes a shape close to the Rust module for easier mapping.

#include <string>
#include <vector>
#include <utility>

#include "../parser.hpp" // uses ::parser::parse_thread

namespace parser {
namespace game_info {

// Mirrors Rust thread meta shape
struct ThreadMeta {
    std::string title;
    std::string author;
    std::string version;
    std::vector<std::string> tags;
};

// Optional page info placeholder (HTML snapshot etc.)
struct PageInfo {
    std::string html;
};

// Link with kind/provider separation
struct Link {
    std::string url;
    std::string kind;      // direct/archive/gofile/etc. (maps from ::parser::LinkInfo::type)
    std::string provider;  // gofile/mega/direct/etc. (maps from ::parser::LinkInfo::provider)
};

// High-level aggregated game info
struct GameInfo {
    ThreadMeta meta;
    std::vector<Link> links;
    PageInfo page;
};

// High-level API that adapts ../parser.hpp output into this module's shapes.
inline GameInfo parse_thread_html(const std::string& html) {
    ::parser::GameInfo gi = ::parser::parse_thread(html);

    GameInfo out;
    out.meta.title   = std::move(gi.meta.title);
    out.meta.author  = std::move(gi.meta.author);
    out.meta.version = std::move(gi.meta.version);
    out.meta.tags    = std::move(gi.meta.tags);
    out.page.html    = html;

    out.links.reserve(gi.links.size());
    for (auto& l : gi.links) {
        Link ln;
        ln.url      = std::move(l.url);
        ln.provider = std::move(l.provider);
        ln.kind     = std::move(l.type);
        out.links.push_back(std::move(ln));
    }
    return out;
}

// Convenience to only extract links from HTML
inline std::vector<Link> parse_links_from_html(const std::string& html) {
    ::parser::GameInfo gi = ::parser::parse_thread(html);
    std::vector<Link> out;
    out.reserve(gi.links.size());
    for (auto& l : gi.links) {
        out.push_back(Link{l.url, l.type, l.provider});
    }
    return out;
}

} // namespace game_info
} // namespace parser
