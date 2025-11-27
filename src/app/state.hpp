#pragma once
// Port target: src/app/state.rs
// Purpose: Central application state (settings, localization, runtime data, selections, etc.)

#include <string>
#include <vector>

#include "../localization/mod.hpp"
#include "../tags/mod.hpp"
#include "settings/settings.hpp"

namespace app {
namespace state {

// Central state snapshot used by UI/runtime.
struct AppState {
    // Effective config (loaded/merged)
    settings::Config config;

    // Localization bundle (messages + locale)
    localization::Bundle i18n;

    // Tags catalog for filters/ui
    tags::Catalog tag_catalog;

    // Quick access mirrors (subset of config for convenience)
    std::string language; // "auto" | "en" | "ru"
    std::vector<std::string> startup_tags;
    std::vector<std::string> startup_exclude_tags;
    std::vector<std::string> startup_prefixes;
    std::vector<std::string> startup_exclude_prefixes;

    bool log_to_file = false;

    // UI selections / transient state (extend as needed)
    std::vector<std::string> active_filters;
};

// Initialize state from loaded configuration and loaded resources.
inline void init(AppState& s, const settings::Config& cfg,
                 const localization::Bundle& bundle,
                 const tags::Catalog& catalog) {
    s.config = cfg;
    s.i18n = bundle;
    s.tag_catalog = catalog;

    s.language = cfg.language;
    s.startup_tags = cfg.startup_tags;
    s.startup_exclude_tags = cfg.startup_exclude_tags;
    s.startup_prefixes = cfg.startup_prefixes;
    s.startup_exclude_prefixes = cfg.startup_exclude_prefixes;
    s.log_to_file = cfg.log_to_file;

    s.active_filters.clear();
}

// Shutdown hook (placeholder for saving transient state, caches, etc.)
inline void shutdown(AppState& /*s*/) {
    // No-op in this lightweight port
}

} // namespace state
} // namespace app
