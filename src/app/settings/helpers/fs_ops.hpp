#pragma once
// Windows-friendly filesystem helpers (header-only)

#include <string>
#include <vector>
#include <cstdio>

#if defined(_WIN32)
#  include <windows.h>
#  include <shlobj.h>
#endif

namespace app {
namespace settings {
namespace helpers {
namespace fs_ops {

// Create directory (and parents) if missing
inline bool ensure_dir(const std::string& path) {
#if defined(_WIN32)
    if (path.empty()) return false;
    std::wstring ws;
    // naive UTF-8 -> UTF-16
    int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), nullptr, 0);
    ws.resize(len);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), &ws[0], len);

    // Create directories step-by-step
    std::wstring current;
    current.reserve(ws.size());
    for (size_t i = 0; i < ws.size(); ++i) {
        wchar_t ch = ws[i];
        current.push_back(ch);
        if (ch == L'\\' || ch == L'/') {
            if (current.size() > 1 && current[current.size() - 2] != L':') {
                CreateDirectoryW(current.c_str(), nullptr);
            }
        }
    }
    // Final segment
    if (!ws.empty() && ws.back() != L'\\' && ws.back() != L'/') {
        CreateDirectoryW(ws.c_str(), nullptr);
    }
    // best-effort
    return true;
#else
    // TODO: non-Windows impl if needed
    return false;
#endif
}

inline bool copy_file(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    auto to_wide = [](const std::string& s) {
        if (s.empty()) return std::wstring();
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        std::wstring ws(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
        return ws;
    };
    std::wstring wf = to_wide(from);
    std::wstring wt = to_wide(to);
    return CopyFileW(wf.c_str(), wt.c_str(), FALSE) != 0;
#else
    return false;
#endif
}

inline bool remove_file(const std::string& path) {
#if defined(_WIN32)
    auto to_wide = [](const std::string& s) {
        if (s.empty()) return std::wstring();
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        std::wstring ws(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
        return ws;
    };
    std::wstring wp = to_wide(path);
    return DeleteFileW(wp.c_str()) != 0;
#else
    return std::remove(path.c_str()) == 0;
#endif
}

} // namespace fs_ops
} // namespace helpers
} // namespace settings
} // namespace app
