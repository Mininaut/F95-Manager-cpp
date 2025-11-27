#include "logger.hpp"
#include <mutex>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
std::mutex& log_mutex() {
    static std::mutex m;
    return m;
}

int& log_level_ref() {
    static int lvl = 0; // 0=INFO,1=WARN,2=ERROR
    return lvl;
}

std::ofstream*& log_file_stream_ref() {
    static std::ofstream* f = nullptr;
    return f;
}

// In-memory log buffer (ring)
static std::vector<std::string>& log_buffer() {
    static std::vector<std::string> b;
    return b;
}
static std::size_t& log_buffer_max() {
    static std::size_t m = 2000; // keep last 2000 lines
    return m;
}

std::string now_timestamp() {
    using namespace std::chrono;
    auto tp = system_clock::now();
    std::time_t tt = system_clock::to_time_t(tp);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &tt);
#else
    localtime_r(&tt, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void write_line(const char* level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex());
    const std::string ts = now_timestamp();
    std::ostringstream line;
    line << "[" << ts << "] " << level << " " << msg << std::endl;

    // stdout
    if (level[1] == 'E') { // [ERROR]
        std::cerr << line.str();
    } else {
        std::cout << line.str();
    }

    // file (if enabled)
    if (auto* pf = log_file_stream_ref()) {
        (*pf) << line.str();
        pf->flush();
    }

    // In-memory ring buffer append
    auto& buf = log_buffer();
    buf.push_back(line.str());
    std::size_t max = log_buffer_max();
    if (buf.size() > max) {
        buf.erase(buf.begin(), buf.begin() + (buf.size() - max));
    }
}
} // namespace

namespace logger {

void set_log_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(log_mutex());
    // Close previous
    if (auto*& pf = log_file_stream_ref()) {
        if (pf->is_open()) {
            pf->flush();
            pf->close();
        }
        delete pf;
        pf = nullptr;
    }
    if (!path.empty()) {
        auto* f = new std::ofstream(path, std::ios::app);
        if (f->is_open()) {
            log_file_stream_ref() = f;
        } else {
            delete f;
            log_file_stream_ref() = nullptr;
            // fall back to console only
        }
    }
}

void set_level(int level) {
    if (level < 0) level = 0;
    if (level > 2) level = 2;
    log_level_ref() = level;
}

void info(const std::string& msg) {
    if (log_level_ref() <= 0) {
        write_line("[INFO] ", msg);
    }
}

void warn(const std::string& msg) {
    if (log_level_ref() <= 1) {
        write_line("[WARN] ", msg);
    }
}

void error(const std::string& msg) {
    // Always print errors
    write_line("[ERROR]", msg);
}

// --- In-memory log buffer API ---
std::vector<std::string> lines() {
    std::lock_guard<std::mutex> lock(log_mutex());
    return log_buffer();
}

void clear() {
    std::lock_guard<std::mutex> lock(log_mutex());
    log_buffer().clear();
}

std::size_t line_count() {
    std::lock_guard<std::mutex> lock(log_mutex());
    return log_buffer().size();
}

} // namespace logger
