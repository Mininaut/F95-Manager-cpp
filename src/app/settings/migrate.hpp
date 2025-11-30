#pragma once
// Port target: src/app/settings/migrate.rs
// Migration helper for moving installed games when extract_dir changes.
// Mirrors Rust migrate_installed_games behavior using filesystem with compatibility.

#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include <system_error>
#include <algorithm>
#include <cstdint>
#include <cctype>

#if defined(__has_include)
  #if __has_include(<filesystem>)
    #include <filesystem>
    #define F95_HAS_FILESYSTEM 1
    namespace fs { using namespace std::filesystem; }
  #elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    #define F95_HAS_FILESYSTEM 1
    namespace fs { using namespace std::experimental::filesystem; }
  #endif
#endif

#ifndef F95_HAS_FILESYSTEM
  #define F95_NO_FILESYSTEM 1
#endif

#ifndef F95_HAS_FILESYSTEM
// Minimal stub so the header still parses on toolchains without filesystem.
// This will be paired with a no-op migrate_installed_games below.
namespace fs {
    struct path {
        std::string s;
        path() = default;
        path(const std::string& v) : s(v) {}
        std::string generic_u8string() const { return s; }
        std::string u8string() const { return s; }
        bool empty() const { return s.empty(); }
        path filename() const { return path(); }
        path parent_path() const { return path(); }
    };
}
#endif

namespace app {
namespace settings {

// tuple: (thread_id, folder, optional_exe_path)
using GameEntry = std::tuple<std::uint64_t, fs::path, std::optional<fs::path>>;
using GameEntryList = std::vector<GameEntry>;

#ifdef F95_HAS_FILESYSTEM
namespace detail_migrate {

inline bool ensure_dir(const fs::path& p) {
    std::error_code ec;
    if (p.empty()) return false;
    if (fs::exists(p, ec)) return true;
    return fs::create_directories(p, ec);
}

inline std::string to_lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return char(std::tolower(c)); });
    return s;
}

// Convert to generic u8 string with forward slashes for robust prefix checks
inline std::string norm_u8(const fs::path& p) {
#if defined(_WIN32)
    std::string s = p.generic_u8string();
    // case-insensitive on Windows
    s = to_lower_copy(s);
    return s;
#else
    return p.generic_u8string();
#endif
}

// Case-insensitive prefix path check (on Windows); case-sensitive elsewhere
inline bool path_starts_with(const fs::path& p, const fs::path& base) {
    std::error_code ec;
    auto ps = norm_u8(fs::weakly_canonical(p, ec));
    if (ec) return false;
    ec.clear();
    auto bs = norm_u8(fs::weakly_canonical(base, ec));
    if (ec) return false;
    if (bs.empty()) return false;
    if (ps.size() < bs.size()) return false;
    if (ps.compare(0, bs.size(), bs) != 0) return false;
    // Ensure boundary at component edge (either end or next is '/')
    return ps.size() == bs.size() || ps[bs.size()] == '/';
}

// Like Rust's strip_prefix; returns relative path if p is inside base; otherwise empty optional
inline std::optional<fs::path> try_strip_prefix(const fs::path& p, const fs::path& base) {
    std::error_code ec;
    auto pre = fs::weakly_canonical(base, ec);
    if (ec) return std::nullopt;
    auto abs = fs::weakly_canonical(p, ec);
    if (ec) return std::nullopt;

    if (!path_starts_with(abs, pre)) return std::nullopt;

    // Compute relative using fs::relative
    auto rel = fs::relative(abs, pre, ec);
    if (ec) return std::nullopt;
    return rel;
}

// If destination exists, add suffixes _movedN up to limit
inline fs::path uniquify_destination(const fs::path& desired_dir, std::size_t limit = 1000) {
    if (!fs::exists(desired_dir)) return desired_dir;
    const auto base_name = desired_dir.filename().u8string();
    const auto parent = desired_dir.parent_path();
    for (std::size_t n = 1; n <= limit; ++n) {
        auto candidate = parent / (fs::path(base_name) += std::string("_moved") + std::to_string(n));
        if (!fs::exists(candidate)) {
            return candidate;
        }
    }
    return desired_dir; // give up; caller may still try to move and fail gracefully
}

// Move directory tree; try rename, fallback to copy+remove
inline bool move_directory_tree(const fs::path& from, const fs::path& to) {
    std::error_code ec;
    // Fast path: try atomic rename
    fs::create_directories(to.parent_path(), ec);
    ec.clear();
    fs::rename(from, to, ec);
    if (!ec) return true;

    // Fallback: copy recursively then remove source
    ec.clear();
    fs::create_directories(to, ec);
    ec.clear();

    const auto opts = fs::copy_options::recursive
                    | fs::copy_options::overwrite_existing;
    fs::copy(from, to, opts, ec);
    if (ec) return false;

    ec.clear();
    fs::remove_all(from, ec);
    return true; // even if remove_all failed partially, we consider move succeeded
}

inline std::optional<fs::path> adjust_exe_under(const fs::path& exe,
                                                const fs::path& old_root,
                                                const fs::path& new_root) {
    if (exe.empty()) return std::nullopt;

    // If exe under old_root, remap to new_root preserving relative
    if (path_starts_with(exe, old_root)) {
        if (auto rel = try_strip_prefix(exe, old_root)) {
            return new_root / *rel;
        } else {
            // Fallback: join filename under new_root
            return new_root / exe.filename();
        }
    }
    return std::nullopt;
}

} // namespace detail_migrate

// C++ port of Rust migrate_installed_games
inline GameEntryList migrate_installed_games(
    const fs::path& old_extract,
    const fs::path& new_extract,
    GameEntryList entries
) {
    using namespace detail_migrate;

    ensure_dir(new_extract);

    GameEntryList moved;

    for (auto& e : entries) {
        const std::uint64_t tid = std::get<0>(e);
        const fs::path old_folder = std::get<1>(e);
        const std::optional<fs::path> exe = std::get<2>(e);

        // Skip missing source folder
        if (!fs::exists(old_folder)) {
            // log: skip missing source
            continue;
        }
        // If already inside new extract dir, keep as-is
        if (path_starts_with(old_folder, new_extract)) {
            moved.emplace_back(tid, old_folder, exe);
            continue;
        }

        // Compute new folder destination
        fs::path new_folder;
        if (auto rel = try_strip_prefix(old_folder, old_extract)) {
            new_folder = new_extract / *rel;
        } else {
            auto name = old_folder.filename();
            new_folder = new_extract / name;
        }

        // Ensure parent exists
        ensure_dir(new_folder.parent_path());

        // Avoid collisions
        new_folder = uniquify_destination(new_folder);

        // Perform move
        if (!move_directory_tree(old_folder, new_folder)) {
            // log: failed to move, skip entry
            continue;
        }

        // Adjust exe path if it was under old folder or under old_extract
        std::optional<fs::path> new_exe;
        if (exe.has_value()) {
            // Case 1: exe under old_folder
            if (auto adj = adjust_exe_under(exe.value(), old_folder, new_folder)) {
                new_exe = *adj;
            }
            // Case 2: exe under old_extract (but not under old_folder)
            else if (auto adj2 = adjust_exe_under(exe.value(), old_extract, new_extract)) {
                new_exe = *adj2;
            } else {
                new_exe = exe.value();
            }
        }

        moved.emplace_back(tid, new_folder, new_exe);
    }

    return moved;
}

#else
// Fallback no-filesystem: simply return entries unchanged (keeps API surface identical)
inline GameEntryList migrate_installed_games(const fs::path&, const fs::path&, GameEntryList entries) {
    return entries;
}
#endif

} // namespace settings
} // namespace app
