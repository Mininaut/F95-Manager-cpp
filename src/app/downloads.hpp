#pragma once
// Downloads manager: queue, progress, cancel. WinHTTP-based streaming into files (header-only impl)

#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <map>
#include <functional>
#include <set>

#if defined(_WIN32)
#  include <windows.h>
#  include <winhttp.h>
#endif

namespace app {
namespace downloads {

struct Item {
    std::string title;
    std::string target_dir;
    std::vector<std::string> urls; // use first that works
    std::uint64_t size_bytes = 0;
};

enum class Status {
    Queued,
    Running,
    Paused,
    Completed,
    Failed,
    Canceled
};

struct Progress {
    std::uint64_t bytes_done = 0;
    std::uint64_t bytes_total = 0;
    Status status = Status::Queued;
    std::string message;
};

namespace detail {
#if defined(_WIN32)
inline std::wstring to_wide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
    return ws;
}
#endif

inline std::string join_path(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    char sep = (a.find('\\') != std::string::npos ? '\\' : '/');
    if (a.back() == '/' || a.back() == '\\') return a + b;
    return a + sep + b;
}

inline std::string filename_from_url(const std::string& url) {
    auto pos = url.find_last_of("/\\");
    if (pos == std::string::npos) return url;
    return url.substr(pos + 1);
}
} // namespace detail

class Manager {
public:
    using Id = std::size_t;

    Manager() : stop_(false) {
        worker_ = std::thread([this]{ this->run(); });
    }

    ~Manager() {
        {
            std::lock_guard<std::mutex> lk(m_);
            stop_ = true;
            cv_.notify_all();
        }
        if (worker_.joinable()) worker_.join();
    }

    Id enqueue(const Item& item) {
        std::lock_guard<std::mutex> lk(m_);
        Id id = ++last_id_;
        items_[id] = item;
        progresses_[id] = Progress{};
        queue_.push_back(id);
        cv_.notify_all();
        return id;
    }

    bool cancel(Id id) {
        std::lock_guard<std::mutex> lk(m_);
        canceled_.insert(id);
        cv_.notify_all();
        return true;
    }

    Progress query(Id id) const {
        std::lock_guard<std::mutex> lk(m_);
        auto it = progresses_.find(id);
        if (it != progresses_.end()) return it->second;
        return Progress{};
    }

private:
    void run() {
        for (;;) {
            Id next = 0;
            {
                std::unique_lock<std::mutex> lk(m_);
                cv_.wait(lk, [this]{ return stop_ || !queue_.empty(); });
                if (stop_) break;
                next = queue_.front();
                queue_.erase(queue_.begin());
                progresses_[next].status = Status::Running;
            }

            download_one(next);

            {
                std::lock_guard<std::mutex> lk(m_);
                if (stop_) break;
            }
        }
    }

    void set_progress(Id id, const std::function<void(Progress&)>& fn) {
        std::lock_guard<std::mutex> lk(m_);
        auto it = progresses_.find(id);
        if (it != progresses_.end()) {
            fn(it->second);
        }
    }

    bool is_canceled(Id id) {
        std::lock_guard<std::mutex> lk(m_);
        return canceled_.count(id) != 0;
    }

    void download_one(Id id) {
        Item it;
        {
            std::lock_guard<std::mutex> lk(m_);
            it = items_[id];
        }

        bool ok_any = false;
        std::string last_err;

        for (const auto& url : it.urls) {
            if (is_canceled(id)) {
                set_progress(id, [](Progress& p){ p.status = Status::Canceled; p.message = "Canceled"; });
                return;
            }
#if defined(_WIN32)
            // Crack URL
            std::wstring urlW = detail::to_wide(url);
            URL_COMPONENTS uc{};
            uc.dwStructSize = sizeof(uc);
            std::wstring scheme(16, L'\0'), host(256, L'\0'), path(2048, L'\0'), extra(2048, L'\0');
            uc.lpszScheme = &scheme[0]; uc.dwSchemeLength = (DWORD)scheme.size();
            uc.lpszHostName = &host[0]; uc.dwHostNameLength = (DWORD)host.size();
            uc.lpszUrlPath  = &path[0]; uc.dwUrlPathLength  = (DWORD)path.size();
            uc.lpszExtraInfo= &extra[0]; uc.dwExtraInfoLength= (DWORD)extra.size();

            if (!WinHttpCrackUrl(urlW.c_str(), 0, 0, &uc)) {
                last_err = "CrackUrl failed";
                continue;
            }
            std::wstring hostW(uc.lpszHostName, uc.dwHostNameLength);
            INTERNET_PORT port = uc.nPort;
            bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);

            std::wstring fullPath;
            fullPath.assign(uc.lpszUrlPath, uc.dwUrlPathLength);
            if (uc.dwExtraInfoLength) fullPath.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);

            HINTERNET hSession = WinHttpOpen(L"F95ManagerCpp/1.0",
                                             WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                             WINHTTP_NO_PROXY_NAME,
                                             WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) { last_err = "Open session failed"; continue; }

            HINTERNET hConnect = WinHttpConnect(hSession, hostW.c_str(), port, 0);
            if (!hConnect) { WinHttpCloseHandle(hSession); last_err = "Connect failed"; continue; }

            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", fullPath.c_str(),
                                                    nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    isHttps ? WINHTTP_FLAG_SECURE : 0);
            if (!hRequest) {
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                last_err = "OpenRequest failed"; continue;
            }

            BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                         WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
            if (!ok) {
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
                last_err = "SendRequest failed"; continue;
            }

            ok = WinHttpReceiveResponse(hRequest, nullptr);
            if (!ok) {
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
                last_err = "ReceiveResponse failed"; continue;
            }

            // Content-Length
            DWORD size = sizeof(DWORD);
            DWORD header_len = 0;
            std::uint64_t content_len = 0;
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &content_len, &size, WINHTTP_NO_HEADER_INDEX);

            set_progress(id, [content_len](Progress& p){ p.bytes_total = content_len; });

            // Prepare file
            std::string filename = it.title.empty() ? detail::filename_from_url(url) : it.title;
            if (filename.empty()) filename = "download.bin";
            std::string out_path = detail::join_path(it.target_dir, filename);
            std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
                last_err = "Open file failed"; continue;
            }

            std::uint64_t done = 0;
            for (;;) {
                if (is_canceled(id)) {
                    out.close();
                    // best-effort: leave partial file
                    set_progress(id, [](Progress& p){ p.status = Status::Canceled; p.message = "Canceled"; });
                    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
                    return;
                }
                DWORD avail = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &avail)) break;
                if (avail == 0) break;
                std::string chunk;
                chunk.resize(avail);
                DWORD read = 0;
                if (!WinHttpReadData(hRequest, &chunk[0], avail, &read)) break;
                chunk.resize(read);
                out.write(chunk.data(), (std::streamsize)chunk.size());
                done += chunk.size();
                set_progress(id, [done](Progress& p){ p.bytes_done = done; p.status = Status::Running; });
            }
            out.close();

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);

            set_progress(id, [](Progress& p){ p.status = Status::Completed; p.message = "Completed"; });
            ok_any = true;
            break; // success
#else
            (void)id;
            (void)url;
#endif
        }

        if (!ok_any) {
            set_progress(id, [last_err](Progress& p){ p.status = Status::Failed; p.message = last_err; });
        }
    }

private:
    mutable std::mutex m_;
    std::condition_variable cv_;
    std::thread worker_;
    std::atomic<bool> stop_;
    Id last_id_ = 0;
    std::vector<Id> queue_;
    std::map<Id, Item> items_;
    std::map<Id, Progress> progresses_;
    std::set<Id> canceled_;
};

// Global singleton-like manager accessor
inline Manager& global() {
    static Manager mgr;
    return mgr;
}

// Convenience API using the global manager
inline Manager::Id enqueue(const Item& item) { return global().enqueue(item); }
inline bool cancel(Manager::Id id) { return global().cancel(id); }
inline Progress query(Manager::Id id) { return global().query(id); }

} // namespace downloads
} // namespace app
