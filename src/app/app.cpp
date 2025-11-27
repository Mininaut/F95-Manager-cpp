#include "app.hpp"
#include <iostream>
#include <string>
#include <fstream>

#include "../logger.hpp"
#include "settings/settings.hpp"
#include "../localization/mod.hpp"
#include "../tags/mod.hpp"
#include "settings/helpers/helpers.hpp"

namespace app {

static bool file_exists(const std::string& path) {
    std::ifstream in(path, std::ios::in);
    return in.good();
}

int App::run() {
    // 1) Load config (JSON). If missing, create defaults.
    settings::Config cfg{};
    const std::string cfgPath = "config.json";
    const bool cfgLoaded = settings::Store::load(cfgPath, cfg);
    if (!cfgLoaded) {
        // Save defaults to create file for the user
        (void)settings::Store::save(cfgPath, cfg);
    }

    // 2) Resolve default folders via helpers if not set
    if (cfg.cache_folder.empty()) {
        std::string base = settings::helpers::paths::app_data_dir();
        if (!base.empty()) {
            if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';
            cfg.cache_folder = base + "cache";
            settings::helpers::fs_ops::ensure_dir(cfg.cache_folder);
        }
    }

    // 3) Init logger: level INFO; optional file logging
    logger::set_level(0);
    if (cfg.log_to_file) {
        // Prefer cache_folder/app.log if provided, fallback to ./app.log
        std::string logPath = "app.log";
        if (!cfg.cache_folder.empty()) {
            logPath = cfg.cache_folder;
            if (!logPath.empty() && logPath.back() != '/' && logPath.back() != '\\') logPath += '/';
            logPath += "app.log";
        }
        logger::set_log_file(logPath);
        logger::info(std::string("Logging to file: ") + logPath);
    }

    logger::info("Startup: F95 Manager C++");

    // 4) Load localization bundle (en/ru or auto->en as a simple default)
    std::string locale = cfg.language == "auto" ? "en" : cfg.language;
    localization::Bundle bundle{};
    bool locOk = localization::load_bundle("src/localization/resources", locale, bundle);
    if (!locOk) {
        locOk = localization::load_bundle("../src/localization/resources", locale, bundle);
    }
    if (!locOk && locale != "en") {
        // Try fallback to en
        locale = "en";
        locOk = localization::load_bundle("src/localization/resources", locale, bundle) ||
                localization::load_bundle("../src/localization/resources", locale, bundle);
    }
    if (locOk) {
        logger::info(std::string("Localization loaded: ") + locale);
    } else {
        logger::warn("Localization load failed, keys will echo");
    }

    // 5) Load tags catalog
    tags::Catalog catalog{};
    bool tagsOk = tags::load_from_json("src/tags/tags.json", catalog);
    if (!tagsOk) {
        tagsOk = tags::load_from_json("../src/tags/tags.json", catalog);
    }
    if (tagsOk) {
        logger::info("Tags loaded: " + std::to_string(catalog.tags.size()) + " tags");
    } else {
        logger::warn("Failed to load tags.json");
    }

    // 6) Print window title localized (as a basic integration demo)
    std::string title = localization::get(bundle, "app-window-title");
    std::cout << title << std::endl;

    // Here would follow: auth, fetch pages, parse, build views, downloads, etc.
    // Step-by-step port can extend from this initialized state.

    return 0;
}

} // namespace app
