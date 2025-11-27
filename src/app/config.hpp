#pragma once
// Port target: src/app/config.rs
// Purpose: Application configuration structures and helpers (merge defaults, validate, etc.)
//
// Notes:
// - This AppConfig mirrors the effective settings exposed to the app layer.
// - It closely follows app::settings::Config to keep parity with the Rust version,
//   but lives separately so higher-level modules can evolve independently.
// - apply_defaults ensures sane defaults and normalization.
// - merge follows \"b overrides a\" semantics (fields present in b replace a).

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace app {
namespace config {

struct AppConfig {
    // Folders
    std::string temp_folder;       // settings-temp-folder
    std::string extract_folder;    // settings-extract-folder
    std::string cache_folder;      // settings-cache-folder

    // Language: "auto" | "en" | "ru"
    std::string language = "auto";

    // Behavior
    bool cache_on_download = true; // settings-cache-on-download
    bool log_to_file = false;      // settings-log-to-file

    // Launch
    std::string custom_launch;     // settings-custom-launch ({{path}} placeholder)

    // Startup filters
    std::vector<std::string> startup_tags;
    std::vector<std::string> startup_exclude_tags;
    std::vector<std::string> startup_prefixes;
    std::vector<std::string> startup_exclude_prefixes;

    // Warnings
    std::vector<std::string> warn_tags;
    std::vector<std::string> warn_prefixes;
};

// Helpers
inline static bool not_empty(const std::string& s) { return !s.empty(); }

// Normalize a path by trimming trailing whitespace and converting backslashes if needed.
// Very conservative normalization (keeps Win-style backslashes).
inline static void normalize_path(std::string& p) {
    // trim
    auto l = p.begin(), r = p.end();
    while (l != r && std::isspace(static_cast<unsigned char>(*l))) ++l;
    while (r != l && std::isspace(static_cast<unsigned char>(*(r - 1)))) --r;
    p.assign(l, r);
    // no further normalization to preserve user intent
}

// Apply defaults and sanity checks.
inline void apply_defaults(AppConfig& cfg) {
    if (cfg.language.empty())
        cfg.language = "auto";
    normalize_path(cfg.temp_folder);
    normalize_path(cfg.extract_folder);
    normalize_path(cfg.cache_folder);
    // cache_on_download/log_to_file already have defaults
}

// Merge b overrides a (like Rust builder pattern or serde merge).
inline AppConfig merge(const AppConfig& a, const AppConfig& b) {
    AppConfig out = a;

    // strings
    if (not_empty(b.temp_folder))        out.temp_folder = b.temp_folder;
    if (not_empty(b.extract_folder))     out.extract_folder = b.extract_folder;
    if (not_empty(b.cache_folder))       out.cache_folder = b.cache_folder;
    if (not_empty(b.language))           out.language = b.language;
    if (not_empty(b.custom_launch))      out.custom_launch = b.custom_launch;

    // bools (explicit overrides)
    out.cache_on_download = b.cache_on_download;
    out.log_to_file       = b.log_to_file;

    // vectors (replace if provided)
    if (!b.startup_tags.empty())               out.startup_tags = b.startup_tags;
    if (!b.startup_exclude_tags.empty())       out.startup_exclude_tags = b.startup_exclude_tags;
    if (!b.startup_prefixes.empty())           out.startup_prefixes = b.startup_prefixes;
    if (!b.startup_exclude_prefixes.empty())   out.startup_exclude_prefixes = b.startup_exclude_prefixes;
    if (!b.warn_tags.empty())                  out.warn_tags = b.warn_tags;
    if (!b.warn_prefixes.empty())              out.warn_prefixes = b.warn_prefixes;

    apply_defaults(out);
    return out;
}

} // namespace config
} // namespace app
