#pragma once
// Port target: src/app/runtime.rs
// Purpose: Runtime services (task scheduling). Lightweight header-only implementation.
// Notes:
// - For parity without adding a global threadpool, schedule() spawns a detached thread.
// - schedule_after() supports optional delay before execution.
// - init()/shutdown()/pump_events() are no-ops in this lightweight port.

#include <functional>
#include <thread>
#include <chrono>
#include <cstddef>
#include <utility>

namespace app {
namespace runtime {

using Task = std::function<void()>;

// Initialize runtime (no-op in this lightweight port).
inline void init(std::size_t /*worker_threads*/ = 0) {}

// Shutdown runtime (no-op in this lightweight port).
inline void shutdown() {}

// Enqueue background task: spawns a detached thread.
inline void schedule(Task task) {
    std::thread(std::move(task)).detach();
}

// Internal helper with delay.
inline void schedule_after_delay(Task task, std::chrono::milliseconds delay) {
    std::thread([t = std::move(task), delay]() mutable {
        if (delay.count() > 0) {
            std::this_thread::sleep_for(delay);
        }
        t();
    }).detach();
}

// Enqueue task with optional delay (default 0ms).
inline void schedule_after(Task task) {
    schedule_after_delay(std::move(task), std::chrono::milliseconds(0));
}

// Pump events (no-op: UI/event systems would hook here if needed).
inline void pump_events() {}

} // namespace runtime
} // namespace app
