#pragma once
// Port target: src/tags/mod.rs
// Purpose: Tags/prefixes loading and access helpers.

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdint>

#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#else
#include "../../vendor/nlohmann/json.hpp"
#endif

namespace tags {

// Basic shapes mirroring Rust types (see src/tags/types.rs)
struct Prefix {
    int id = 0;
    std::string name;
    std::string css_class;
};

struct Group {
    int id = 0;
    std::string name;
    std::vector<Prefix> prefixes;
};

using TagMap = std::unordered_map<int, std::string>;

struct Catalog {
    // groups like Engine/Status/etc.
    std::vector<Group> games;
    std::vector<Group> comics;
    std::vector<Group> animations;
    std::vector<Group> assets;

    // tags like { id -> name }
    TagMap tags;

    // whether options are enabled (parity with Rust tags::Tags.options)
    bool options = false;
};

bool load_from_json(const std::string& path, Catalog& out);
inline const std::string* tag_name(const Catalog& cat, int id) {
    auto it = cat.tags.find(id);
    if (it == cat.tags.end()) return nullptr;
    return &it->second;
}

// Convenience helpers mirroring Rust-style lookups
inline std::string tag_name_by_id(const Catalog& cat, std::uint32_t id) {
    auto it = cat.tags.find(static_cast<int>(id));
    if (it == cat.tags.end()) return std::string();
    return it->second;
}

inline std::string prefix_name_by_id(const Catalog& cat, std::uint32_t id) {
    const auto find_in = [&](const std::vector<Group>& groups) -> std::string {
        for (const auto& g : groups) {
            for (const auto& p : g.prefixes) {
                if (static_cast<std::uint32_t>(p.id) == id) {
                    return p.name;
                }
            }
        }
        return {};
    };
    if (auto n = find_in(cat.games); !n.empty()) return n;
    if (auto n = find_in(cat.comics); !n.empty()) return n;
    if (auto n = find_in(cat.animations); !n.empty()) return n;
    if (auto n = find_in(cat.assets); !n.empty()) return n;
    return {};
}

// Implementation
inline bool load_from_json(const std::string& path, Catalog& out) {
    out = Catalog{};
    std::ifstream in(path, std::ios::in);
    if (!in.is_open()) {
        return false;
    }
    try {
        nlohmann::json root;
        in >> root;

        // tags: map string->string with numeric keys as strings
        if (root.contains("tags") && root["tags"].is_object()) {
            for (auto it = root["tags"].begin(); it != root["tags"].end(); ++it) {
                int id = 0;
                try { id = std::stoi(it.key()); } catch (...) { continue; }
                if (it.value().is_string()) {
                    out.tags[id] = it.value().get<std::string>();
                }
            }
        }

        // helper lambda to load groups
        auto load_groups = [](const nlohmann::json& jarr, std::vector<Group>& dst) {
            if (!jarr.is_array()) return;
            for (const auto& g : jarr) {
                Group gg;
                if (g.contains("id")) gg.id = g["id"].get<int>();
                if (g.contains("name")) gg.name = g["name"].get<std::string>();
                if (g.contains("prefixes") && g["prefixes"].is_array()) {
                    for (const auto& p : g["prefixes"]) {
                        Prefix pr;
                        if (p.contains("id")) pr.id = p["id"].get<int>();
                        if (p.contains("name")) pr.name = p["name"].get<std::string>();
                        if (p.contains("class")) pr.css_class = p["class"].get<std::string>();
                        gg.prefixes.push_back(std::move(pr));
                    }
                }
                dst.push_back(std::move(gg));
            }
        };

        if (root.contains("prefixes") && root["prefixes"].is_object()) {
            const auto& pref = root["prefixes"];
            if (pref.contains("games"))      load_groups(pref["games"], out.games);
            if (pref.contains("comics"))     load_groups(pref["comics"], out.comics);
            if (pref.contains("animations")) load_groups(pref["animations"], out.animations);
            if (pref.contains("assets"))     load_groups(pref["assets"], out.assets);
        }

        // options flag (bool) from JSON root
        if (root.contains("options") && root["options"].is_boolean()) {
            out.options = root["options"].get<bool>();
        }

        return true;
    } catch (...) {
        return false;
    }
}

} // namespace tags
