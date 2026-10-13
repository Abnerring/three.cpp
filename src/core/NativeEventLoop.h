#pragma once
#include "core/JSValue.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace NativeEventLoop_detail {
struct Mailbox;
}

// The cross-thread envelope has only native storage. In particular it cannot
// contain opaque payloads, callbacks, shared_ptr<void>, or managed JSValue handles.
struct NativeEventData {
    std::uint64_t kind = 0;
    std::uint64_t token = 0;
    std::int64_t code = 0;
    std::vector<std::byte> bytes;
    std::string text;
    std::vector<std::pair<std::string, std::string>> fields;
};

// Host scheduling foundation for native loaders, not a DOM event loop.
// All operations except Sender::post/wake and isOwnerThread require the creating
// thread. Destruction also requires that thread, outside this loop's active pump.
// Owner APIs reject reentry from native payload finalizers during destruction.
// Managed callbacks, receivers, arguments, and captures must belong to that
// thread's runtime heap; native primitives may cross thread boundaries.
// Native clocks must be finite, monotonic milliseconds and must not execute JS.
class NativeEventLoop {
  public:
    using Id = std::uint64_t;
    using Clock = std::function<double()>;
    using NativeHandler = void (*)(const JSValue& receiver, const NativeEventData& data,
                                   const std::vector<JSValue>& captures);
    class Sender {
      public:
        Sender() = default;
        // Returns zero for a destroyed loop or an unregistered receiver. Accepted
        // posts receive globally unique IDs; delivery may subsequently be canceled.
        Id post(Id receiver, NativeEventData data) const;
        // Wakes a host wait, including after advancing an injected native clock.
        void wake() const;
      private:
        explicit Sender(std::weak_ptr<NativeEventLoop_detail::Mailbox> mailbox)
            : mailbox_(std::move(mailbox)) {}
        std::weak_ptr<NativeEventLoop_detail::Mailbox> mailbox_;
        friend class NativeEventLoop;
    };
    enum class ErrorPhase { BeforeTask, Task, AfterTask };
    struct Failure {
        ErrorPhase phase;
        std::exception_ptr error;
    };
    struct RunResult {
        Id task = 0;
        std::vector<Failure> failures;
        bool ranTask() const { return task != 0; }
    };

    explicit NativeEventLoop(Clock clock = {});
    ~NativeEventLoop();
    NativeEventLoop(const NativeEventLoop&) = delete;
    NativeEventLoop& operator=(const NativeEventLoop&) = delete;
    NativeEventLoop(NativeEventLoop&&) = delete;
    NativeEventLoop& operator=(NativeEventLoop&&) = delete;

    bool isOwnerThread() const noexcept;
    Sender sender() const;
    Id postTask(const JSValue& callback, const JSValue& receiver = {},
                const std::vector<JSValue>& arguments = {});
    // Native API: finite nonnegative delay, without WebIDL coercion, nesting
    // clamps, or background throttling. Zero delay is a separate FIFO task.
    Id setTimeout(const JSValue& callback, double delayMilliseconds = 0,
                  const JSValue& receiver = {}, const std::vector<JSValue>& arguments = {});
    // Removes pending callbacks/timers/worker envelopes immediately. A running
    // task is already consumed and cannot be canceled.
    bool cancelTask(Id task);
    Id registerReceiver(NativeHandler handler, const JSValue& receiver = {},
                        const std::vector<JSValue>& captures = {});
    // Discards all queued envelopes for this receiver and releases its roots.
    bool unregisterReceiver(Id receiver);

    // Entry checkpoint is native host pumping policy: it completes the previous
    // host turn before selecting a task. Each task also ends with a checkpoint.
    // Errors are returned in phase order, including when a task and checkpoint
    // both fail. Other queued tasks remain runnable. JSJobs currently reports
    // only the first error within any one microtask checkpoint.
    // With no native task ready, one thread-wide FinalizationRegistry cleanup job
    // runs as a separate host task followed by its own microtask checkpoint.
    // Nested pumping from microtask or cleanup jobs returns an empty result on
    // this thread, across loop objects.
    RunResult runOneTask();
    // This loop's native tasks and thread-wide cleanup jobs; not microtasks.
    bool hasReadyTasks();
    // This loop's tasks/timers plus the thread-wide shared JSJobs queue.
    bool hasPendingTasks() const;
    // Returns immediately for shared pending microtasks or cleanup jobs without
    // executing them.
    // A wake can return false before the timeout. Real waiting is bounded;
    // fake clocks advance explicitly.
    bool waitForWork(std::chrono::milliseconds maximumWait);

  private:
    struct State;
    std::unique_ptr<State> state_;
    void requireOwnerThread() const;
};
