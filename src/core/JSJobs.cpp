#include "core/JSJobs.h"
#include <deque>
#include <exception>

namespace {
struct Jobs {
    std::deque<JSValue> pending;
    bool running = false;
};
thread_local Jobs jobs;
}
void queueMicrotask(JSValue callback) {
    if (!callback.isCallable()) throw JSTypeError("Microtask callback must be callable");
    jobs.pending.push_back(std::move(callback));
}
bool hasPendingMicrotasks() { return !jobs.pending.empty(); }
bool isDrainingMicrotasks() { return jobs.running; }
std::size_t drainMicrotasks() {
    if (jobs.running) return 0;
    struct Guard {
        Guard() { jobs.running = true; }
        ~Guard() { jobs.running = false; }
    } guard;
    std::size_t count = 0;
    std::exception_ptr firstError;
    while (!jobs.pending.empty()) {
        auto callback = std::move(jobs.pending.front());
        jobs.pending.pop_front();
        ++count;
        try { callback.call(); }
        catch (...) { if (!firstError) firstError = std::current_exception(); }
    }
    if (firstError) std::rethrow_exception(firstError);
    return count;
}
