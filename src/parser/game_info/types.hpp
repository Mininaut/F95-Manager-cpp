#pragma once
// Port target: src/parser/game_info/types.rs
// ThreadId, Platform bitflags, and PlatformDownloads equivalents.

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include "../parser.hpp"

namespace parser {
namespace game_info {


struct F95Page {
    std::string url;
};

struct ThreadId {
    std::uint64_t value{0};

    std::uint64_t get() const { return value; }

    F95Page get_page() const {
        return F95Page{ "https://f95zone.to/threads/" + std::to_string(value) + "/" };
    }
};

// Bitflags-style platform enum (mirrors Rust bitflags)
enum class Platform : std::uint8_t {
    NONE    = 0,
    WINDOWS = 0b00001,
    LINUX   = 0b00010,
    MAC     = 0b00100,
    ANDROID = 0b01000,
    OTHER   = 0b10000,
};

// bitwise operators for Platform flags
inline Platform operator|(Platform a, Platform b) {
    return static_cast<Platform>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}
inline Platform operator&(Platform a, Platform b) {
    return static_cast<Platform>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}
inline Platform& operator|=(Platform& a, Platform b) {
    a = a | b; return a;
}

namespace detail {
    inline std::string to_lower_copy(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return char(std::tolower(c)); });
        return s;
    }
    inline std::string trim_copy(const std::string& s) {
        size_t b = 0, e = s.size();
        while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
        return s.substr(b, e - b);
    }
}

// Parse platform flags from a free-form string (mirrors Rust From<&str> for Platform)
inline Platform platform_from_string(std::string value) {
    using namespace detail;
    std::string lower = to_lower_copy(std::move(value));

    // Normalize common delimiters into '/'
    for (char& c : lower) {
        if (c == '\\' || c == ',' || c == '|' || c == '&') c = '/';
    }

    Platform flags = Platform::NONE;

    size_t start = 0;
    while (start < lower.size()) {
        size_t sep = lower.find('/', start);
        std::string token = trim_copy(lower.substr(start, sep == std::string::npos ? std::string::npos : sep - start));
        if (!token.empty()) {
            if (token.find("win") != std::string::npos || token == "pc") {
                flags |= Platform::WINDOWS;
            }
            if (token.find("linux") != std::string::npos) {
                flags |= Platform::LINUX;
            }
            if (token.find("mac") != std::string::npos || token.find("osx") != std::string::npos) {
                flags |= Platform::MAC;
            }
            if (token.find("android") != std::string::npos) {
                flags |= Platform::ANDROID;
            }
            // Note: Rust impl doesn't auto-set OTHER; keep parity by not setting it here either
        }
        if (sep == std::string::npos) break;
        start = sep + 1;
    }

    return flags;
}

struct PlatformDownloads {
    Platform platform{Platform::NONE};
    std::vector<::parser::LinkInfo> links;

    PlatformDownloads() = default;
    PlatformDownloads(Platform p, std::vector<::parser::LinkInfo> l)
        : platform(p), links(std::move(l)) {}
};

} // namespace game_info
} // namespace parser
