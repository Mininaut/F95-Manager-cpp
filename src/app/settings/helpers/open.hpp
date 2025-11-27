#pragma once
// Windows-friendly open helpers (header-only)

#include <string>

#if defined(_WIN32)
#  include <windows.h>
#  include <shellapi.h>
#endif

namespace app {
namespace settings {
namespace helpers {
namespace open {

inline std::wstring to_wide(const std::string& s) {
#if defined(_WIN32)
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
    return ws;
#else
    return std::wstring();
#endif
}

// Open a file or directory in Windows Explorer.
// Returns true on success.
inline bool in_explorer(const std::string& path) {
#if defined(_WIN32)
    std::wstring args = L"\"";
    args += to_wide(path);
    args += L"\"";
    HINSTANCE h = ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(h) > 32;
#else
    return false;
#endif
}


// Open a URL in default browser (Windows). Returns true on success.
inline bool url(const std::string& link) {
#if defined(_WIN32)
    std::wstring w = to_wide(link);
    HINSTANCE h = ShellExecuteW(nullptr, L"open", w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(h) > 32;
#else
    return false;
#endif
}

} // namespace open
} // namespace helpers
} // namespace settings
} // namespace app
