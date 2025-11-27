#pragma once
// Game download orchestrator bridging to app::downloads queue
// Mirrors Rust high-level downloader but reuses the C++ global queue implementation.

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

#include "../app/downloads.hpp"

namespace game_download {

struct DownloadLink {
    std::string url;
    std::string provider; // mirrors Rust: direct/archive/gofile/etc. (informational)
};

struct Progress {
    std::uint64_t bytes_total = 0;
    std::uint64_t bytes_done  = 0;
    bool finished = false;
    bool failed   = false;
    std::string error;
};

class Downloader {
public:
    // Start download into target_dir, return true if started successfully.
    // The first working link will be used by the underlying manager.
    bool start(const std::vector<DownloadLink>& links, const std::string& target_dir) {
        if (links.empty() || target_dir.empty()) {
            return false;
        }
        // Build queue item
        app::downloads::Item it;
        it.title = ""; // let filename be inferred from URL by downloads manager
        it.target_dir = target_dir;
        it.urls.reserve(links.size());
        for (const auto& l : links) {
            it.urls.push_back(l.url);
        }

        id_ = app::downloads::enqueue(it);
        running_ = true;
        return true;
    }

    // Poll progress (if applicable)
    Progress progress() const {
        Progress out{};
        if (!running_) return out;

        auto p = app::downloads::query(id_);
        out.bytes_done = p.bytes_done;
        out.bytes_total = p.bytes_total;

        using S = app::downloads::Status;
        switch (p.status) {
            case S::Completed:
                out.finished = true;
                out.failed = false;
                break;
            case S::Failed:
                out.finished = true;
                out.failed = true;
                out.error = p.message;
                break;
            case S::Canceled:
                out.finished = true;
                out.failed = true;
                out.error = "Canceled";
                break;
            default:
                // Running/Queued/Paused
                out.finished = false;
                out.failed = false;
                out.error.clear();
                break;
        }
        return out;
    }

    // Request cancellation.
    void cancel() {
        if (!running_) return;
        (void)app::downloads::cancel(id_);
        running_ = false;
    }

private:
    app::downloads::Manager::Id id_ = 0;
    bool running_ = false;
};

} // namespace game_download
