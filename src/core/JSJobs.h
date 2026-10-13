#pragma once
#include "core/JSValue.h"
#include <cstddef>
#include <limits>
#include <exception>

// The native event loop calls this checkpoint on the JSValue owning thread.
// Worker threads must post native data to that loop, never JSValue handles.
void queueMicrotask(JSValue callback);
bool hasPendingMicrotasks();
// True while this thread is executing a microtask checkpoint.
bool isDrainingMicrotasks();
std::size_t drainMicrotasks();
// End one host job after its complete microtask checkpoint. WeakRef construction
// and successful deref keep their target alive until this boundary.
void endJSJob();
// Finalization cleanup is an asynchronous host task, separate from microtasks.
// The weak queue cancels tasks whose registry/runtime no longer exists.
bool hasPendingFinalizationJobs();
bool isDrainingFinalizationJobs();
struct FinalizationJobFailure {
    bool microtaskCheckpoint;
    std::exception_ptr error;
};
// A host can collect callback/checkpoint failures separately instead of throwing
// the first failure. Each cleanup job still completes its microtask checkpoint.
std::size_t drainFinalizationJobs(std::size_t maximumJobs = std::numeric_limits<std::size_t>::max(),
                                  std::vector<FinalizationJobFailure>* failures = nullptr);
// Runtime-internal enqueue; only the collector and cleanup retry use this.
void queueFinalizationRegistryCleanup(JSWeakValue registry);
