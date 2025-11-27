#pragma once
// Networking/parsing helpers used by fetch module.
// Real implementation using WinHTTP (Windows) and simple string utilities.

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#if defined(_WIN32)
#  include <windows.h>
#  include <winhttp.h>
#endif

namespace app {
namespace fetch {
namespace helpers {

// Simple key-value headers representation
using Headers = std::map<std::string, std::string>;

// Placeholder request/response shapes
struct HttpRequest {
    std::string url;
    std::string method = "GET";
    Headers headers;
    std::string body;
};

struct HttpResponse {
    int status = 0;
    Headers headers;
    std::string body;
};

// utils
inline std::string trim(const std::string& s) {
    auto b = s.begin(), e = s.end();
    while (b != e && std::isspace(static_cast<unsigned char>(*b))) ++b;
    while (e != b && std::isspace(static_cast<unsigned char>(*(e - 1)))) --e;
    return std::string(b, e);
}

inline std::string url_join(const std::string& base, const std::string& path) {
    if (base.empty()) return path;
    if (path.empty()) return base;
    bool slash = (!base.empty() && base.back() == '/');
    bool lead = (!path.empty() && path.front() == '/');
    if (slash && lead) return base + path.substr(1);
    if (!slash && !lead) return base + "/" + path;
    return base + path;
}

#if defined(_WIN32)

// narrow<->wide helpers
inline std::wstring to_wide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &ws[0], len);
    return ws;
}

inline std::string from_wide(const std::wstring& ws) {
    if (ws.empty()) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()), &s[0], len, nullptr, nullptr);
    return s;
}

// Convert UTF-8 headers map to WinHTTP headers string (CRLF separated)
inline std::wstring build_headers(const Headers& h) {
    std::wstring result;
    for (const auto& kv : h) {
        std::wstring k = to_wide(kv.first);
        std::wstring v = to_wide(kv.second);
        if (!result.empty()) result.append(L"\r\n");
        result.append(k).append(L": ").append(v);
    }
    return result;
}

#endif // _WIN32

inline HttpResponse http_request(const HttpRequest& req) {
    HttpResponse resp;

#if defined(_WIN32)
    // Parse URL
    std::wstring urlW = to_wide(req.url);
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    std::wstring scheme(16, L'\0'), host(256, L'\0'), path(2048, L'\0'), extra(2048, L'\0');
    uc.lpszScheme = &scheme[0]; uc.dwSchemeLength = static_cast<DWORD>(scheme.size());
    uc.lpszHostName = &host[0]; uc.dwHostNameLength = static_cast<DWORD>(host.size());
    uc.lpszUrlPath  = &path[0]; uc.dwUrlPathLength  = static_cast<DWORD>(path.size());
    uc.lpszExtraInfo= &extra[0]; uc.dwExtraInfoLength= static_cast<DWORD>(extra.size());

    if (!WinHttpCrackUrl(urlW.c_str(), 0, 0, &uc)) {
        resp.status = 0;
        return resp;
    }
    std::wstring hostW(uc.lpszHostName, uc.dwHostNameLength);
    INTERNET_PORT port = uc.nPort;
    bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);

    std::wstring fullPath;
    fullPath.assign(uc.lpszUrlPath, uc.dwUrlPathLength);
    if (uc.dwExtraInfoLength) {
        fullPath.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);
    }

    // Open session
    HINTERNET hSession = WinHttpOpen(L"F95ManagerCpp/1.0",
                                     WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { resp.status = 0; return resp; }

    // Connect
    HINTERNET hConnect = WinHttpConnect(hSession, hostW.c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); resp.status = 0; return resp; }

    // Request
    std::wstring methodW = to_wide(req.method.empty() ? "GET" : req.method);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, methodW.c_str(), fullPath.c_str(),
                                            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            isHttps ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        resp.status = 0;
        return resp;
    }

    // Add headers
    std::wstring headersW = build_headers(req.headers);
    if (!headersW.empty()) {
        WinHttpAddRequestHeaders(hRequest, headersW.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
    }

    // Send
    const void* bodyData = req.body.empty() ? nullptr : static_cast<const void*>(req.body.data());
    DWORD bodySize = req.body.empty() ? 0 : static_cast<DWORD>(req.body.size());

    BOOL ok = WinHttpSendRequest(hRequest,
                                 WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 const_cast<void*>(bodyData), bodySize,
                                 bodySize, 0);
    if (!ok) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        resp.status = 0;
        return resp;
    }

    ok = WinHttpReceiveResponse(hRequest, nullptr);
    if (!ok) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        resp.status = 0;
        return resp;
    }

    // Status code
    DWORD statusCode = 0, size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
    resp.status = static_cast<int>(statusCode);

    // Read body
    std::string buffer;
    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &avail)) break;
        if (avail == 0) break;
        std::string chunk;
        chunk.resize(avail);
        DWORD read = 0;
        if (!WinHttpReadData(hRequest, &chunk[0], avail, &read)) break;
        chunk.resize(read);
        buffer.append(chunk);
    }
    resp.body = std::move(buffer);

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
#else
    // Non-Windows: no implementation here
    resp.status = 0;
#endif
    return resp;
}

} // namespace helpers
} // namespace fetch
} // namespace app
