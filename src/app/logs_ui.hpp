#pragma once
// Port target: src/app/logs_ui.rs
// Purpose: Logs window/panel UI, autoscroll, copy/clear actions.
// C++ implementation wraps the global logger backend to provide a simple API
// matching the Rust module responsibilities.

#include <string>
#include <vector>
#include <utility>
#include <cstddef>
#include <algorithm>

#include "../logger.hpp"

namespace app {
namespace logs_ui {

struct LogEntry {
    std::string level; // "INFO", "WARN", "ERROR"
    std::string text;  // message text (timestamp removed)
};

// Heuristic parser for logger line format produced by logger.cpp:
// "[YYYY-mm-dd HH:MM:SS] [INFO] message...\n"
inline LogEntry parse_line(const std::string& line) {
    LogEntry e;
    // Find second bracketed token for level
    // Expected: '['ts']' ' ' '['level']' ' ' message
    std::size_t pos1 = line.find(']');
    if (pos1 == std::string::npos) {
        e.level = "INFO";
        e.text = line;
        return e;
    }
    // skip space
    std::size_t pos2 = line.find('[', pos1 + 1);
    std::size_t pos3 = std::string::npos;
    if (pos2 != std::string::npos) {
        pos3 = line.find(']', pos2 + 1);
    }
    if (pos2 != std::string::npos && pos3 != std::string::npos) {
        // Extract level between brackets and strip possible spaces/brackets artifacts
        std::string lvl = line.substr(pos2 + 1, pos3 - pos2 - 1);
        // lvl like "INFO" or "ERROR"
        e.level = lvl;
        // message starts after a space following level bracket (if any)
        std::size_t msg_start = pos3 + 1;
        if (msg_start < line.size() && line[msg_start] == ' ') msg_start++;
        e.text = (msg_start < line.size() ? line.substr(msg_start) : std::string());
    } else {
        // Fallback
        e.level = "INFO";
        e.text = (pos1 + 1 < line.size() ? line.substr(pos1 + 1) : std::string());
    }
    // Trim trailing newline(s)
    while (!e.text.empty() && (e.text.back() == '\n' || e.text.back() == '\r')) {
        e.text.pop_back();
    }
    return e;
}

// Append a log entry via the logger backend
inline void add_entry(const LogEntry& e) {
    // Normalize level to upper
    std::string lvl = e.level;
    std::transform(lvl.begin(), lvl.end(), lvl.begin(), [](unsigned char c){ return (char)std::toupper(c); });
    if (lvl == "ERROR") {
        logger::error(e.text);
    } else if (lvl == "WARN" || lvl == "WARNING") {
        logger::warn(e.text);
    } else {
        logger::info(e.text);
    }
}

// Clear the in-memory buffer
inline void clear() {
    logger::clear();
}

// Snapshot current entries (parsed)
inline std::vector<LogEntry> current() {
    std::vector<std::string> ls = logger::lines();
    std::vector<LogEntry> out;
    out.reserve(ls.size());
    for (auto& s : ls) {
        out.push_back(parse_line(s));
    }
    return out;
}

// Convenience: number of current lines
inline std::size_t count() {
    return logger::line_count();
}

} // namespace logs_ui
} // namespace app
