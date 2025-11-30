#pragma once
// Port target: src/parser/game_info/thread_meta.rs
// Purpose: Extract rich thread metadata (title, version, author, cover, screenshots, tag ids)
// from an HTML thread page. This mirrors the Rust structure and keeps API simple.

#include <string>
#include <vector>
#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstdint>

#include "../parser.hpp"        // reuse extract_title/extract_author/extract_version helpers
#include "../../tags/mod.hpp"   // for tags::Catalog

namespace parser {
namespace game_info {

struct ThreadMeta {
    std::string title;
    std::string cover;
    std::vector<std::string> screens;
    std::vector<std::uint32_t> tag_ids;
    std::string creator; // author
    std::string version;
};

// Internal helpers (regex utils)
namespace detail_tm {

// find all matches capturing group 'group'
inline std::vector<std::string> regex_all(const std::string& s, const std::regex& re, int group = 1) {
    std::vector<std::string> out;
    std::sregex_iterator it(s.begin(), s.end(), re), end;
    for (; it != end; ++it) {
        if (it->size() > group) out.push_back((*it)[group].str());
    }
    return out;
}

inline std::string regex_first(const std::string& s, const std::regex& re, int group = 1) {
    std::smatch m;
    if (std::regex_search(s, m, re)) {
        if (m.size() > group) return m[group].str();
    }
    return {};
}

inline std::string trim(const std::string& in) {
    size_t b = 0, e = in.size();
    while (b < e && std::isspace(static_cast<unsigned char>(in[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1]))) --e;
    return in.substr(b, e - b);
}

inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

} // namespace detail_tm

// Extract ThreadMeta from thread HTML using patterns similar to the Rust implementation.
// If a cover isn't found explicitly, the first screenshot (attachment) is used when available.
// Tag ids are resolved by name using the provided tags::Catalog (lowercase match).
inline ThreadMeta extract_thread_meta_from_html(const std::string& html, const tags::Catalog* catalog = nullptr) {
    using namespace detail_tm;

    ThreadMeta tm;

    // Title / version / creator best-effort using existing parser helpers
    tm.title   = parser::extract_title(html);
    tm.creator = parser::extract_author(html);
    tm.version = parser::extract_version(html);

    // Screenshots (attachments): https://attachments.f95zone.to/2025/08/5195249_....png
    // Accept optional query string.
    static const std::regex RE_ATTACH(
        R"re(href="(https://attachments\.f95zone\.to/\d+/\d+/\d+_[A-Za-z0-9_\-]+\.[A-Za-z0-9]+(?:\?[^\s"'<>]*)?)")re",
        std::regex::icase);
    static const std::regex RE_COVER(
        R"re(src="(https://attachments\.f95zone\.to/\d+/\d+/\d+_[A-Za-z0-9_\-]+\.[A-Za-z0-9]+(?:\?[^\s"'<>]*)?)")re",
        std::regex::icase);

    {
        std::unordered_set<std::string> seen;
        for (const auto& s : regex_all(html, RE_ATTACH, 1)) {
            if (seen.insert(s).second) tm.screens.push_back(s);
        }
    }

    // Cover: prefer explicit cover; fallback to first screenshot if available
    tm.cover = regex_first(html, RE_COVER, 1);
    if (tm.cover.empty() && !tm.screens.empty()) {
        tm.cover = tm.screens.front();
    }

    // Tag ids from tag list block
    if (catalog) {
        // Build reverse map: lowercase name -> id
        std::unordered_map<std::string, std::uint32_t> reverse;
        reverse.reserve(catalog->tags.size());
        for (const auto& kv : catalog->tags) {
            std::string lname = lower(kv.second);
            reverse.emplace(std::move(lname), static_cast<std::uint32_t>(kv.first));
        }

        // Capture inner of <span class="js-tagList"> ... </span>
        static const std::regex RE_TAG_BLOCK(R"re((?is)<span class="js-tagList">(.+?)</span>)re");
        static const std::regex RE_TAG_TEXT(R"re(>([^<>]+)<)re");

        std::string block = regex_first(html, RE_TAG_BLOCK, 1);
        if (!block.empty()) {
            std::unordered_set<std::uint32_t> added;
            auto names = regex_all(block, RE_TAG_TEXT, 1);
            for (auto& name : names) {
                std::string trimmed = trim(name);
                if (trimmed.empty()) continue;
                std::string lname = lower(trimmed);
                auto it = reverse.find(lname);
                if (it != reverse.end()) {
                    std::uint32_t id = it->second;
                    if (added.insert(id).second) tm.tag_ids.push_back(id);
                }
            }
        }
    }

    return tm;
}

} // namespace game_info
} // namespace parser
