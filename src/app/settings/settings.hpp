#pragma once
// Settings handling (port of Rust src/app/settings/*)
// - Config structures and serialization (JSON via nlohmann::json)
// - Store: load/save
// NOTE: header-only implementation for simplicity

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// single-header json in vendor/nlohmann/json.hpp (added in CMake include dirs)
#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#else
#include "../../../vendor/nlohmann/json.hpp"
#endif

namespace app {
namespace settings {

struct Config {
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

// Apply reasonable defaults. You can refine paths from helpers later.
inline void apply_defaults(Config& c) {
    if (c.language.empty()) c.language = "auto";
    // Default folders remain empty; can be resolved via helpers/paths later.
}

class Store {
public:
    // Load config from persistent storage (JSON). Always sets 'out' (merged with defaults).
    // Returns true if file existed and was parsed successfully, false if file missing or parse error.
    static bool load(const std::string& path, Config& out);

    // Save config (JSON).
    static bool save(const std::string& path, const Config& cfg);
};

// --------- JSON adapters ----------
inline void to_json(nlohmann::json& j, const Config& c) {
    j = nlohmann::json{
        {"temp_folder", c.temp_folder},
        {"extract_folder", c.extract_folder},
        {"cache_folder", c.cache_folder},
        {"language", c.language},
        {"cache_on_download", c.cache_on_download},
        {"log_to_file", c.log_to_file},
        {"custom_launch", c.custom_launch},
        {"startup_tags", c.startup_tags},
        {"startup_exclude_tags", c.startup_exclude_tags},
        {"startup_prefixes", c.startup_prefixes},
        {"startup_exclude_prefixes", c.startup_exclude_prefixes},
        {"warn_tags", c.warn_tags},
        {"warn_prefixes", c.warn_prefixes}
    };
}

inline void from_json(const nlohmann::json& j, Config& c) {
    // keep defaults first
    Config tmp = c;

    if (j.contains("temp_folder")) j.at("temp_folder").get_to(tmp.temp_folder);
    if (j.contains("extract_folder")) j.at("extract_folder").get_to(tmp.extract_folder);
    if (j.contains("cache_folder")) j.at("cache_folder").get_to(tmp.cache_folder);

    if (j.contains("language")) j.at("language").get_to(tmp.language);

    if (j.contains("cache_on_download")) j.at("cache_on_download").get_to(tmp.cache_on_download);
    if (j.contains("log_to_file")) j.at("log_to_file").get_to(tmp.log_to_file);

    if (j.contains("custom_launch")) j.at("custom_launch").get_to(tmp.custom_launch);

    if (j.contains("startup_tags")) j.at("startup_tags").get_to(tmp.startup_tags);
    if (j.contains("startup_exclude_tags")) j.at("startup_exclude_tags").get_to(tmp.startup_exclude_tags);
    if (j.contains("startup_prefixes")) j.at("startup_prefixes").get_to(tmp.startup_prefixes);
    if (j.contains("startup_exclude_prefixes")) j.at("startup_exclude_prefixes").get_to(tmp.startup_exclude_prefixes);

    if (j.contains("warn_tags")) j.at("warn_tags").get_to(tmp.warn_tags);
    if (j.contains("warn_prefixes")) j.at("warn_prefixes").get_to(tmp.warn_prefixes);

    c = std::move(tmp);
}

// --------- Store implementation ----------
inline bool Store::load(const std::string& path, Config& out) {
    // Prepare defaults first
    Config cfg;
    apply_defaults(cfg);

    // Try open file
    std::ifstream in(path, std::ios::in);
    if (!in.is_open()) {
        // No file: return false, but 'out' gets defaults
        out = std::move(cfg);
        return false;
    }

    try {
        nlohmann::json j;
        in >> j;
        from_json(j, cfg);
        apply_defaults(cfg);
        out = std::move(cfg);
        return true;
    } catch (...) {
        // Parse error: keep defaults
        out = std::move(cfg);
        return false;
    }
}

inline bool Store::save(const std::string& path, const Config& cfg) {
    try {
        nlohmann::json j = cfg;
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out.is_open()) return false;
        out << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace settings
} // namespace app
