#pragma once
// High-level fetch API that wraps helpers::http_request and integrates with parser.

#include <string>
#include <vector>
#include <map>

#include "helpers.hpp"
#include "../../parser/parser.hpp"
#include "../../logger.hpp"

namespace app {
namespace fetch {

// Convenience alias
using Headers = helpers::Headers;

// GET url and return body as string; fills status code
inline std::string get_body(const std::string& url, int& out_status, const Headers& headers = {}) {
    helpers::HttpRequest req;
    req.url = url;
    req.method = "GET";
    req.headers = headers;

    auto resp = helpers::http_request(req);
    out_status = resp.status;
    if (resp.status >= 200 && resp.status < 300) {
        return resp.body;
    }
    logger::warn("GET failed: " + url + " status=" + std::to_string(resp.status));
    return {};
}

// Fetch a thread page and parse it into GameInfo
inline parser::GameInfo fetch_and_parse_thread(const std::string& url, const Headers& headers = {}) {
    int status = 0;
    std::string html = get_body(url, status, headers);
    if (status < 200 || status >= 300 || html.empty()) {
        logger::error("Failed to fetch thread: " + url);
        return parser::GameInfo{};
    }
    return parser::parse_thread(html);
}

} // namespace fetch
} // namespace app
