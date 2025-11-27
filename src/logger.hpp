#pragma once
#include <string>
#include <vector>

namespace logger {

// Initialize logging to optionally write to a file in addition to stdout.
void set_log_file(const std::string& path);   // empty path disables file logging
void set_level(int level);                    // 0=INFO,1=WARN,2=ERROR

// Log APIs
void info(const std::string& msg);
void warn(const std::string& msg);
void error(const std::string& msg);

// In-memory ring buffer APIs (for Logs UI parity)
// Returns a copy of current log lines (thread-safe snapshot).
std::vector<std::string> lines();

// Clears the in-memory buffer (does not affect file/stdout).
void clear();

// Current number of lines in the in-memory buffer.
std::size_t line_count();

} // namespace logger
