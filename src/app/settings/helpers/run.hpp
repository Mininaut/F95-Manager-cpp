#pragma once
// Windows-friendly process execution helper (header-only)

#include <string>
#include <vector>

#if defined(_WIN32)
#  include <windows.h>
#endif

namespace app {
namespace settings {
namespace helpers {
namespace run {

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

// Execute a command with arguments. Returns process exit code (or -1 on failure).
inline int exec(const std::string& cmd, const std::vector<std::string>& args) {
#if defined(_WIN32)
    // Build command line: "cmd" "arg1" "arg2" ...
    std::wstring cl = L"\"";
    cl += to_wide(cmd);
    cl += L"\"";
    for (const auto& a : args) {
        cl += L" \"";
        cl += to_wide(a);
        cl += L"\"";
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    DWORD createFlags = 0;

    std::wstring cl_mut = cl; // CreateProcess requires modifiable buffer
    BOOL ok = CreateProcessW(
        nullptr,
        &cl_mut[0],
        nullptr, nullptr,
        FALSE,
        createFlags,
        nullptr,
        nullptr,
        &si,
        &pi
    );
    if (!ok) {
        return -1;
    }

    // Wait for process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(exitCode);
#else
    (void)cmd; (void)args;
    return -1;
#endif
}

} // namespace run
} // namespace helpers
} // namespace settings
} // namespace app
