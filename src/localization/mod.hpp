#pragma once
// Localization loader/dispatcher using Fluent-like .ftl resources in resources/*.ftl
// Minimal parser: supports lines "key = value", ignores comments/blank lines.
// Basic placeholder passthrough: keeps { $var } as-is (no param expansion API here).

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace localization {

using Messages = std::unordered_map<std::string, std::string>;

struct Bundle {
    Messages messages;
    std::string locale; // "en", "ru", etc.
};

namespace detail {

inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
}
inline std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}
inline std::string trim(std::string s) { return rtrim(ltrim(std::move(s))); }

inline bool parse_ftl_line(const std::string& line, std::string& out_key, std::string& out_val) {
    // Skip comments and blanks
    std::string s = trim(line);
    if (s.empty()) return false;
    if (s.rfind("#", 0) == 0) return false;

    // Find key = value
    auto pos = s.find('=');
    if (pos == std::string::npos) return false;

    out_key = trim(s.substr(0, pos));
    out_val = trim(s.substr(pos + 1));

    // Support simple multiline continuation if value ends with backslash \
    // (not Fluent spec, just convenience)
    return !out_key.empty();
}

} // namespace detail

inline bool load_bundle(const std::string& locale_dir, const std::string& locale, Bundle& out) {
    out = Bundle{};
    out.locale = locale;

    // Expect file like resources/en.ftl
    std::string p = locale_dir;
    if (!p.empty() && p.back() != '/' && p.back() != '\\') p += '/';
    p += locale + ".ftl";

    std::ifstream in(p, std::ios::in);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    std::string last_key;
    while (std::getline(in, line)) {
        std::string key, val;
        if (detail::parse_ftl_line(line, key, val)) {
            out.messages[key] = val;
            last_key = key;
        } else {
            // Handle multiline values: indent indicates continuation in Fluent,
            // here we approximate by appending non-comment non-empty lines when a key was set.
            std::string t = detail::trim(line);
            if (!last_key.empty() && !t.empty() && t.rfind("#", 0) != 0) {
                out.messages[last_key] += "\n" + t;
            }
        }
    }
    return true;
}

inline std::string get(const Bundle& bundle, const std::string& key) {
    auto it = bundle.messages.find(key);
    if (it != bundle.messages.end()) return it->second;
    // Fallback: return key if missing
    return key;
}

} // namespace localization
