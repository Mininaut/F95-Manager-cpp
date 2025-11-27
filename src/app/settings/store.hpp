#pragma once
// Port target: src/app/settings/store.rs
// Purpose: Persistence layer for settings (load/save, paths, defaults).
// This header provides a thin facade over settings::Store with default path helpers.

#include <string>

#include "settings.hpp"
#include "helpers/paths.hpp"
#include "helpers/fs_ops.hpp"

namespace app {
namespace settings {
namespace store {

namespace detail {
inline std::string join_path(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    char sep = (a.find('\\') != std::string::npos ? '\\' : '/');
    if (a.back() == '/' || a.back() == '\\') return a + b;
    return a + sep + b;
}
} // namespace detail

// Returns app data directory used for config persistence.
// On Windows: %APPDATA%\F95-Manager-cpp (created if missing)
// Fallback: empty on failure.
inline std::string default_dir() {
    std::string dir = helpers::paths::app_data_dir();
    if (!dir.empty()) {
        (void)helpers::fs_ops::ensure_dir(dir);
    }
    return dir;
}

// Returns default config.json path under app data dir.
// If app data dir cannot be resolved, fallback to "config.json" in CWD.
inline std::string default_path() {
    std::string dir = default_dir();
    if (dir.empty()) return "config.json";
    return detail::join_path(dir, "config.json");
}

// Load config from given path. Always writes 'out' (defaults if parse fails).
// Returns true on successful parse, false on missing/parse error.
inline bool load_from(const std::string& path, Config& out) {
    return Store::load(path, out);
}

// Save config to given path. Returns true on success.
inline bool save_to(const std::string& path, const Config& cfg) {
    return Store::save(path, cfg);
}

// Load config from default_path(). Returns true on success (file existed + parsed).
inline bool load_default(Config& out) {
    return load_from(default_path(), out);
}

// Save config to default_path(). Returns true on success.
inline bool save_default(const Config& cfg) {
    return save_to(default_path(), cfg);
}

} // namespace store
} // namespace settings
} // namespace app
