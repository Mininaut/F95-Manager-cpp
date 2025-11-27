#pragma once
// Windows-friendly paths helpers (header-only)

#include <string>

#if defined(_WIN32)
#  include <windows.h>
#  include <shlobj.h>
#endif

#include "fs_ops.hpp"

namespace app {
namespace settings {
namespace helpers {
namespace paths {

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

inline std::string from_wide(const std::wstring& ws) {
#if defined(_WIN32)
    if (ws.empty()) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &s[0], len, nullptr, nullptr);
    return s;
#else
    return std::string();
#endif
}

// %APPDATA%\F95-Manager-cpp (ensure exist)
inline std::string app_data_dir() {
#if defined(_WIN32)
    PWSTR pPath = nullptr;
    std::string result;
    // Roaming AppData
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pPath)) && pPath) {
        std::wstring base(pPath);
        CoTaskMemFree(pPath);
        if (!base.empty() && base.back() != L'\\' && base.back() != L'/') base += L'\\';
        base += L"F95-Manager-cpp";
        result = from_wide(base);
        fs_ops::ensure_dir(result);
        return result;
    }
    return std::string();
#else
    return std::string();
#endif
}

// %USERPROFILE%\Downloads (or KnownFolder Downloads)
inline std::string downloads_dir() {
#if defined(_WIN32)
    PWSTR pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &pPath)) && pPath) {
        std::wstring w(pPath);
        CoTaskMemFree(pPath);
        return from_wide(w);
    }
    return std::string();
#else
    return std::string();
#endif
}

} // namespace paths
} // namespace helpers
} // namespace settings
} // namespace app
