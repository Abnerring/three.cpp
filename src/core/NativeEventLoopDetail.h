#pragma once
#include <atomic>
#include <cstdint>

// Implementation details, exposed separately only for deterministic regression
// checks. They are not part of the loader or event-loop application API.
namespace NativeEventLoop_detail {
// Zero and UINT64_MAX are reserved. Exhaustion never wraps or reuses an ID.
std::uint64_t takeId(std::atomic<std::uint64_t>& next);
// Initialization assigns a never-reused token on this thread's first loop.
std::uint64_t initializeThreadIdentity();
// Inspection never initializes an identity or accesses a managed heap.
std::uint64_t currentThreadIdentity() noexcept;
}
