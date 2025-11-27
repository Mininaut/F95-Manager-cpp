#pragma once
// TODO: Port helper utilities from Rust (src/app/settings/helpers/*)
// Map Rust helpers to C++ equivalents:
// - fs_ops.rs -> file system operations (create dirs, copy, remove, etc.)
// - paths.rs  -> path resolution, user dirs
// - open.rs   -> open file/dir in OS
// - run.rs    -> spawn processes (if needed)

#include <string>
#include <vector>

// bring in header-only implementations
#include "fs_ops.hpp"
#include "paths.hpp"
#include "open.hpp"
#include "run.hpp"

namespace app {
namespace settings {
namespace helpers {

namespace fs_ops {
    bool ensure_dir(const std::string& path);
    bool copy_file(const std::string& from, const std::string& to);
    bool remove_file(const std::string& path);
}

namespace paths {
    std::string app_data_dir();
    std::string downloads_dir();
}

namespace open {
    bool in_explorer(const std::string& path);
}

namespace run {
    int exec(const std::string& cmd, const std::vector<std::string>& args);
}

} // namespace helpers
} // namespace settings
} // namespace app
