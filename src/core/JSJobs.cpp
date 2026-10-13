#include "core/JSJobs.h"
#include "core/JSFinalizationRegistry.h"
#include <deque>
#include <list>
#include <exception>

namespace {
struct Jobs {
    std::deque<JSValue> pending;
    bool running = false;
    std::list<JSWeakValue> finalization;
    std::list<JSWeakValue> activeFinalization;
    bool finalizing = false;
    bool invokingCleanup = false;
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
    if (jobs.running || jobs.invokingCleanup) return 0;
    struct Guard {
        Guard() { jobs.running = true; }
        ~Guard() { endJSJob(); jobs.running = false; }
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
void endJSJob() { JSValue::clearKeptObjects(); }
void queueFinalizationRegistryCleanup(JSWeakValue registry) { jobs.finalization.push_back(std::move(registry)); }
bool hasPendingFinalizationJobs() {
    std::erase_if(jobs.finalization, [](const auto& registry) { return registry.expired(); });
    std::erase_if(jobs.activeFinalization, [](const auto& registry) { return registry.expired(); });
    return !jobs.finalization.empty() || !jobs.activeFinalization.empty();
}
bool isDrainingFinalizationJobs() { return jobs.finalizing; }
std::size_t drainFinalizationJobs(std::size_t maximumJobs, std::vector<FinalizationJobFailure>* failures) {
    if (jobs.running || jobs.finalizing || maximumJobs == 0) return 0;
    struct Guard {
        Guard() { jobs.finalizing = true; }
        ~Guard() {
            // Preserve the current turn's remaining jobs ahead of retries or
            // new jobs queued by a callback, including on the exception path.
            jobs.finalization.splice(jobs.finalization.begin(), jobs.activeFinalization);
            jobs.finalizing = false;
        }
    } guard;
    endJSJob();
    std::size_t count = 0;
    std::exception_ptr firstError;
    // Jobs created by a cleanup callback/retry belong to the next host turn.
    jobs.activeFinalization.swap(jobs.finalization);
    while (!jobs.activeFinalization.empty() && count < maximumJobs) {
        auto pending = std::move(jobs.activeFinalization.front()); jobs.activeFinalization.pop_front();
        const auto registry = pending.lock();
        if (!registry) continue;
        ++count;
        try {
            struct CleanupGuard {
                CleanupGuard() { jobs.invokingCleanup = true; }
                ~CleanupGuard() { jobs.invokingCleanup = false; }
            } cleanupGuard;
            JSFinalizationRegistry::cleanup(*registry);
        }
        catch (...) {
            if (failures) failures->push_back({false,std::current_exception()});
            else if (!firstError) firstError = std::current_exception();
        }
        try { drainMicrotasks(); }
        catch (...) {
            if (failures) failures->push_back({true,std::current_exception()});
            else if (!firstError) firstError = std::current_exception();
        }
        endJSJob();
    }
    if (firstError) std::rethrow_exception(firstError);
    return count;
}
