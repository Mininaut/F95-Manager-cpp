#pragma once
// High-level parser API (lightweight, regex-based) to approximate Rust parser behavior.
// Parses basic thread metadata (title, author, version, tags) and link extraction.

#include <string>
#include <vector>
#include <regex>
#include <algorithm>

namespace parser {

inline std::string trim(const std::string& s) {
    auto b = s.begin(), e = s.end();
    while (b != e && std::isspace(static_cast<unsigned char>(*b))) ++b;
    while (e != b && std::isspace(static_cast<unsigned char>(*(e - 1)))) --e;
    return std::string(b, e);
}

inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

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

struct ThreadMeta {
    std::string title;
    std::string author;
    std::string version;
    std::vector<std::string> tags;
};

struct LinkInfo {
    std::string url;
    std::string provider; // e.g. gofile/mega/direct/etc.
    std::string type;     // e.g. direct/archive/info/gofile
};

struct GameInfo {
    ThreadMeta meta;
    std::vector<LinkInfo> links;
};

// naive HTML title extraction
inline std::string extract_title(const std::string& html) {
    // <title>...</title>
    std::string t = regex_first(html, std::regex("<title[^>]*>(.*?)</title>", std::regex::icase), 1);
    if (!t.empty()) return trim(t);
    // <h1>...</h1> as fallback
    t = regex_first(html, std::regex("<h1[^>]*>(.*?)</h1>", std::regex::icase), 1);
    return trim(t);
}

// try to find "Author: XYZ" or rel span
inline std::string extract_author(const std::string& html) {
    std::string a = regex_first(html, std::regex("Author\\s*:\\s*([^<\\n\\r]+)", std::regex::icase), 1);
    if (!a.empty()) return trim(a);
    // Try meta tags
    a = regex_first(html, std::regex("<meta[^>]*name=[\"']author[\"'][^>]*content=[\"']([^\"']+)[\"'][^>]*>", std::regex::icase), 1);
    return trim(a);
}

// try to find "Version: XYZ" or "vX.Y"
inline std::string extract_version(const std::string& html) {
    // Prefer explicit "Version: X[.Y]" entries where X[.Y] is numeric-only.
    // If multiple, take the last numeric-only occurrence. Otherwise, fallback to "vX.Y" pattern.
    std::vector<std::string> vers = regex_all(html, std::regex("Version\\s*:\\s*([^<\\n\\r]+)", std::regex::icase), 1);
    std::regex numeric_only("^\\s*(\\d+(?:\\.\\d+)*)\\s*$");
    for (auto it = vers.rbegin(); it != vers.rend(); ++it) {
        std::smatch m;
        std::string candidate = trim(*it);
        if (std::regex_match(candidate, m, numeric_only)) {
            return trim(m[1].str());
        }
    }
    // Fallback: find the last "vX.Y" style.
    std::vector<std::string> vtags = regex_all(html, std::regex("\\bv(\\d+(?:\\.\\d+)*)\\b", std::regex::icase), 1);
    if (!vtags.empty()) {
        return trim(vtags.back());
    }
    // As a last resort return the last "Version:" value trimmed.
    if (!vers.empty()) {
        return trim(vers.back());
    }
    return {};
}

// tags from anchors (<a class="tag">text</a>) or 'data-tag="text"'
inline std::vector<std::string> extract_tags(const std::string& html) {
    std::vector<std::string> out = regex_all(html, std::regex("<a[^>]*class=[\"'][^\"']*tag[^\"']*[\"'][^>]*>(.*?)</a>", std::regex::icase), 1);
    auto extra = regex_all(html, std::regex("data-tag=[\"']([^\"']+)[\"']", std::regex::icase), 1);
    out.insert(out.end(), extra.begin(), extra.end());
    // normalize/trim
    for (auto& t : out) t = trim(t);
    // dedup
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

inline std::string classify_provider(const std::string& url) {
    auto l = lower(url);
    if (l.find("gofile") != std::string::npos) return "gofile";
    if (l.find("mega.nz") != std::string::npos) return "mega";
    if (l.find("pixeldrain") != std::string::npos) return "pixeldrain";
    if (l.find("mediafire") != std::string::npos) return "mediafire";
    if (l.find("drive.google") != std::string::npos) return "gdrive";
    if (l.find("anonfiles") != std::string::npos) return "anonfiles";
    if (l.find("rapidgator") != std::string::npos) return "rapidgator";
    if (l.find("ddownload") != std::string::npos || l.find("f95zone") != std::string::npos) return "direct";
    return "direct";
}

inline std::vector<LinkInfo> extract_links(const std::string& html) {
    std::vector<LinkInfo> out;
    auto urls = regex_all(html, std::regex("href\\s*=\\s*\"(https?://[^\"]+)\"", std::regex::icase), 1);
    for (auto& u : urls) {
        LinkInfo li;
        li.url = u;
        li.provider = classify_provider(u);
        // try to guess 'type' by provider or extension
        if (li.provider == "gofile") li.type = "gofile";
        else if (li.provider == "direct") {
            if (u.find(".zip") != std::string::npos || u.find(".7z") != std::string::npos || u.find(".rar") != std::string::npos)
                li.type = "archive";
            else
                li.type = "direct";
        } else {
            li.type = "download";
        }
        out.push_back(std::move(li));
    }
    return out;
}

inline GameInfo parse_thread(const std::string& html) {
    GameInfo gi;
    gi.meta.title   = extract_title(html);
    gi.meta.author  = extract_author(html);
    gi.meta.version = extract_version(html);
    gi.meta.tags    = extract_tags(html);
    gi.links        = extract_links(html);
    return gi;
}

} // namespace parser
