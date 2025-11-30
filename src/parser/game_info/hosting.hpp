#pragma once
// Port target: src/parser/game_info/hosting.rs
// Purpose: Mirror Hosting/HostingSubset enums and basic URL classification utilities.

#include <string>
#include <vector>
#include <algorithm>

namespace parser {
namespace game_info {

enum class Hosting {
    Pixeldrain,
    Gofile,
    Mega,
    Catbox,
    Mediafire,
    Workupload,
    Uploadhaven,
    Racaty,
    Zippy,
    Nopy,
    Mixdrop,
};

enum class HostingSubset {
    Pixeldrain,
    Gofile,
    Mega,
    Catbox,
};

// Lowercase copy
inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

// Base scheme like Rust's Hosting::base()
inline const char* base() {
    return "https://";
}

inline const char* to_domain(Hosting h) {
    switch (h) {
        case Hosting::Gofile:      return "gofile.io";
        case Hosting::Mediafire:   return "mediafire.com";
        case Hosting::Mega:        return "mega.nz";
        case Hosting::Mixdrop:     return "mixdrop.sn";
        case Hosting::Nopy:        return "nopy.to";
        case Hosting::Pixeldrain:  return "pixeldrain.com";
        case Hosting::Racaty:      return "racaty.com";
        case Hosting::Uploadhaven: return "uploadhaven.com";
        case Hosting::Workupload:  return "workupload.com";
        case Hosting::Zippy:       return "zippyshare.com";
        case Hosting::Catbox:      return "files.catbox.moe";
        default:                   return "";
    }
}

// Extract the second-level label (e.g., "mega" from "mega.nz")
inline std::string second_level_from_domain(const std::string& domain) {
    // split by '.'
    std::vector<std::string> parts;
    std::string acc;
    for (char c : domain) {
        if (c == '.') {
            if (!acc.empty()) parts.push_back(acc);
            acc.clear();
        } else {
            acc.push_back(c);
        }
    }
    if (!acc.empty()) parts.push_back(acc);
    if (parts.size() >= 2) {
        return parts[parts.size() - 2];
    }
    return domain;
}

inline std::string extract_domain(const std::string& url) {
    // naive extraction: after "://" until '/' or end
    std::string u = url;
    auto pos = u.find("://");
    size_t start = (pos == std::string::npos) ? 0 : pos + 3;
    size_t end = u.find('/', start);
    if (end == std::string::npos) end = u.size();
    return u.substr(start, end - start);
}

// Try classify Hosting from URL. Returns true if mapped.
inline bool try_from_url(const std::string& url, Hosting& out) {
    std::string dom = lower(extract_domain(url));
    std::string core = second_level_from_domain(dom); // e.g. "mega"
    if (core == "gofile")      { out = Hosting::Gofile; return true; }
    if (core == "mediafire")   { out = Hosting::Mediafire; return true; }
    if (core == "mega")        { out = Hosting::Mega; return true; }
    if (core == "mixdrop")     { out = Hosting::Mixdrop; return true; }
    if (core == "nopy")        { out = Hosting::Nopy; return true; }
    if (core == "pixeldrain")  { out = Hosting::Pixeldrain; return true; }
    if (core == "racaty")      { out = Hosting::Racaty; return true; }
    if (core == "uploadhaven") { out = Hosting::Uploadhaven; return true; }
    if (core == "workupload")  { out = Hosting::Workupload; return true; }
    if (core == "zippyshare")  { out = Hosting::Zippy; return true; }
    if (core == "catbox")      { out = Hosting::Catbox; return true; }
    return false;
}

// Convert Hosting to subset (if applicable). Returns true on success.
inline bool to_subset(Hosting h, HostingSubset& subset) {
    switch (h) {
        case Hosting::Pixeldrain: subset = HostingSubset::Pixeldrain; return true;
        case Hosting::Gofile:     subset = HostingSubset::Gofile;     return true;
        case Hosting::Mega:       subset = HostingSubset::Mega;       return true;
        case Hosting::Catbox:     subset = HostingSubset::Catbox;     return true;
        default: return false;
    }
}

// Try classify subset directly from URL. Returns true for supported subset hostings.
inline bool try_subset_from_url(const std::string& url, HostingSubset& subset) {
    Hosting h;
    if (!try_from_url(url, h)) return false;
    return to_subset(h, subset);
}

} // namespace game_info
} // namespace parser
