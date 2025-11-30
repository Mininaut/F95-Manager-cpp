#pragma once
// Port target: src/app/cache.rs
// Purpose: Caching layer for metadata and images during downloads.
// Simple header-only cache that stores files by key inside a cache directory.
// Keys are mapped to filenames by a safe encode (non-alnum -> '_') to avoid path issues.

#include <string>
#include <vector>
#include <cctype>

#include "settings/helpers/fs_ops.hpp"

#if defined(_WIN32)
#  include <windows.h>
#endif

namespace app {
namespace cache {

inline std::string& cache_root_mut() {
    static std::string root;
    return root;
}

inline std::string sanitize_key(const std::string& key) {
    std::string out;
    out.reserve(key.size());
    for (unsigned char c : key) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.') out.push_back((char)c);
        else out.push_back('_');
    }
    if (out.empty()) out = "_";
    return out;
}

#if defined(_WIN32)
inline std::wstring to_wide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
    return ws;
}
inline bool file_exists(const std::string& path) {
    std::wstring w = to_wide(path);
    DWORD attr = GetFileAttributesW(w.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}
inline bool remove_file_win(const std::string& path) {
    return app::settings::helpers::fs_ops::remove_file(path);
}
#else
inline bool file_exists(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}
inline bool remove_file_win(const std::string& path) {
    return std::remove(path.c_str()) == 0;
}
#endif

// Initialize cache directory, creating it if necessary.
inline bool init(const std::string& cache_dir) {
    cache_root_mut() = cache_dir;
    if (cache_root_mut().empty()) return false;
    return app::settings::helpers::fs_ops::ensure_dir(cache_root_mut());
}

// Compute absolute path for a cached key
inline std::string path_for(const std::string& key) {
    if (cache_root_mut().empty()) return std::string();
    std::string fname = sanitize_key(key);
    std::string p = cache_root_mut();
    if (!p.empty() && p.back() != '/' && p.back() != '\\') p += '/';
    p += fname;
    return p;
}

// Put: copy a file from data_path into cache under key
inline bool put(const std::string& key, const std::string& data_path) {
    if (cache_root_mut().empty()) return false;
    std::string dst = path_for(key);
    if (dst.empty()) return false;
    // Ensure root exists
    if (!app::settings::helpers::fs_ops::ensure_dir(cache_root_mut())) return false;
    return app::settings::helpers::fs_ops::copy_file(data_path, dst);
}

// Get: return path to cached file if exists, else empty string
inline std::string get(const std::string& key) {
    std::string p = path_for(key);
    if (!p.empty() && file_exists(p)) return p;
    return {};
}

// Remove: delete cached file for key
inline bool remove(const std::string& key) {
    std::string p = path_for(key);
    if (p.empty()) return false;
    if (!file_exists(p)) return true;
    return remove_file_win(p);
}

} // namespace cache
} // namespace app
