#pragma once
// Port target: src/app/logs_ui.rs
// Purpose: Logs window/panel UI, autoscroll, copy/clear actions.

#include <string>
#include <vector>

namespace app {
namespace logs_ui {

struct LogEntry {
    std::string level; // "INFO", "WARN", "ERROR"
    std::string text;
};

// Placeholder API:
// void add_entry(const LogEntry& e);
// void clear();
// std::vector<LogEntry> current();

} // namespace logs_ui
} // namespace app
