#pragma once
// Port target: src/parser/game_info/link/mod.rs
// Aggregates different link providers/types parsing.

#include <string>
#include <algorithm>

#include "../../parser.hpp" // reuse provider classification

namespace parser {
namespace game_info {
namespace link {

struct LinkInfo {
    std::string url;
    std::string provider; // archive/direct/gofile/etc.
    std::string name;     // optional display name
};

// Infer a simple display name from URL (filename or host)
inline std::string infer_name_from_url(const std::string& url) {
    // try last path segment
    auto slash = url.find_last_of("/\\");
    std::string tail = (slash == std::string::npos) ? url : url.substr(slash + 1);
    if (!tail.empty()) return tail;

    // fallback host
    auto scheme = url.find("://");
    auto host_start = (scheme == std::string::npos) ? 0 : scheme + 3;
    auto host_end = url.find('/', host_start);
    if (host_end == std::string::npos) host_end = url.size();
    if (host_end > host_start) return url.substr(host_start, host_end - host_start);

    return url;
}

// High-level dispatch: classify provider and infer name
inline LinkInfo parse(const std::string& url) {
    LinkInfo li;
    li.url = url;
    li.provider = ::parser::classify_provider(url);
    li.name = infer_name_from_url(url);
    return li;
}

} // namespace link
} // namespace game_info
} // namespace parser
