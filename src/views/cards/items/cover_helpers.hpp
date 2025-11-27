#pragma once
// Port target: src/views/cards/items/cover_helpers.rs
// Purpose: Helpers for engine resolution and warnings collection based on tags/prefixes.

#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <cctype>

#include "../../../parser/parser.hpp"
#include "../../../app/settings/settings.hpp"
#include "../../../tags/mod.hpp"

namespace views {
namespace cards {
namespace items {
namespace cover_helpers {

inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

inline bool iequals(const std::string& a, const std::string& b) {
    return to_lower(a) == to_lower(b);
}

inline bool icontains(const std::string& hay, const std::string& needle) {
    auto h = to_lower(hay);
    auto n = to_lower(needle);
    return h.find(n) != std::string::npos;
}

inline std::string unescape_html_entities(const std::string& s) {
    // minimal: handle &#039; -> '
    std::string out = s;
    const std::string from = "&#039;";
    const std::string to = "'";
    size_t pos = 0;
    while ((pos = out.find(from, pos)) != std::string::npos) {
        out.replace(pos, from.size(), to);
        pos += to.size();
    }
    return out;
}

// Try to resolve engine name by matching known Engine prefixes against thread tags/title.
// This is a heuristic because C++ port does not carry explicit prefix ids like Rust.
inline std::string resolve_engine_name(const ::parser::GameInfo& gi, const tags::Catalog& cat) {
    // Locate "Engine" group within games prefixes
    const tags::Group* engine_group = nullptr;
    for (const auto& g : cat.games) {
        if (iequals(g.name, "Engine")) { engine_group = &g; break; }
    }
    if (!engine_group) return std::string();

    // 1) Match by tag equality
    for (const auto& p : engine_group->prefixes) {
        for (const auto& t : gi.meta.tags) {
            if (iequals(p.name, t)) {
                return unescape_html_entities(p.name);
            }
        }
    }
    // 2) Fallback: substring in title
    for (const auto& p : engine_group->prefixes) {
        if (icontains(gi.meta.title, p.name)) {
            return unescape_html_entities(p.name);
        }
    }
    return std::string();
}

// Collect warnings (tag and prefix names) based on user settings.
// - Tags: name-based intersection with gi.meta.tags
// - Prefixes: name-based match (appears in tags or title) against configured warn_prefixes
inline std::pair<std::vector<std::string>, std::vector<std::string>>
collect_warnings(const ::parser::GameInfo& gi,
                 const app::settings::Config& cfg,
                 const tags::Catalog& cat) {
    std::vector<std::string> tag_names;
    std::vector<std::string> pref_names;

    // Tags: compare by names (case-insensitive)
    for (const auto& warn : cfg.warn_tags) {
        for (const auto& t : gi.meta.tags) {
            if (iequals(warn, t)) {
                tag_names.push_back(t);
                break;
            }
        }
    }

    // Build a flat list of all prefixes (games, comics, animations, assets)
    auto scan_groups = [&](const std::vector<tags::Group>& groups) {
        for (const auto& g : groups) {
            for (const auto& p : g.prefixes) {
                // If this prefix is configured as warned, check presence
                bool configured = std::any_of(cfg.warn_prefixes.begin(), cfg.warn_prefixes.end(),
                                              [&](const std::string& w){ return iequals(w, p.name); });
                if (!configured) continue;

                // If present in tags list or in title, add
                bool present = std::any_of(gi.meta.tags.begin(), gi.meta.tags.end(),
                                           [&](const std::string& t){ return iequals(t, p.name); })
                               || icontains(gi.meta.title, p.name);

                if (present) {
                    pref_names.push_back(unescape_html_entities(p.name));
                }
            }
        }
    };
    scan_groups(cat.games);
    scan_groups(cat.comics);
    scan_groups(cat.animations);
    scan_groups(cat.assets);

    return {tag_names, pref_names};
}

} // namespace cover_helpers
} // namespace items
} // namespace cards
} // namespace views
