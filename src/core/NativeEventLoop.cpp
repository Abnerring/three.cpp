#include "core/NativeEventLoop.h"
#include "core/NativeEventLoopDetail.h"
#include "core/JSJobs.h"
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace NativeEventLoop_detail {
struct Envelope {
    std::uint64_t task = 0;
    std::uint64_t receiver = 0; // zero denotes an owner-thread managed task
    NativeEventData data;
};
struct Mailbox {
    std::mutex mutex;
    std::condition_variable changed;
    bool open = true;
    std::uint64_t wakeRevision = 0;
    std::deque<Envelope> ready;
    std::unordered_set<std::uint64_t> pending;
    std::unordered_set<std::uint64_t> receivers;
};
std::uint64_t takeId(std::atomic<std::uint64_t>& next) {
    auto value = next.load(std::memory_order_relaxed);
    for (;;) {
        if (value == 0 || value == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("Native event IDs exhausted");
        if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) return value;
    }
}
namespace {
std::atomic<std::uint64_t> nextThreadIdentity{1};
thread_local std::uint64_t threadIdentity = 0;
}
std::uint64_t initializeThreadIdentity() {
    if (threadIdentity == 0) threadIdentity = takeId(nextThreadIdentity);
    return threadIdentity;
}
std::uint64_t currentThreadIdentity() noexcept { return threadIdentity; }
}
namespace {
std::atomic<std::uint64_t> nextId{1};
thread_local bool pumping = false;
using Envelope = NativeEventLoop_detail::Envelope;
using Mailbox = NativeEventLoop_detail::Mailbox;
using Id = NativeEventLoop::Id;
double nativeNow() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
void enqueueLocked(Mailbox& mailbox, Envelope envelope) {
    const auto task = envelope.task;
    mailbox.pending.insert(task);
    try { mailbox.ready.push_back(std::move(envelope)); }
    catch (...) { mailbox.pending.erase(task); throw; }
}
void validateInputs(const JSValue& first, const JSValue& receiver,
                    const std::vector<JSValue>& values) {
    // JSWeakValue checks heap identity before dereferencing a managed record.
    // Perform every check before JSValue copies retain any foreign record.
    static_cast<void>(JSWeakValue(first));
    static_cast<void>(JSWeakValue(receiver));
    for (const auto& value : values) static_cast<void>(JSWeakValue(value));
}
}
struct NativeEventLoop::State {
    struct Callback {
        JSValue function;
        JSValue receiver;
        std::vector<JSValue> arguments;
        std::optional<std::pair<double, Id>> timer;
    };
    struct Receiver {
        NativeHandler handler;
        JSValue receiver;
        std::vector<JSValue> captures;
    };
    const std::uint64_t owner = NativeEventLoop_detail::initializeThreadIdentity();
    bool running = false;
    bool closing = false;
    Clock clock;
    std::optional<double> lastTime;
    std::shared_ptr<Mailbox> mailbox = std::make_shared<Mailbox>();
    std::unordered_map<Id, Callback> callbacks;
    std::unordered_map<Id, Receiver> receivers;
    std::map<std::pair<double, Id>, Id> timers;
    explicit State(Clock source) : clock(source ? std::move(source) : Clock(nativeNow)) {}
    double now() {
        const auto time = clock();
        if (!std::isfinite(time) || (lastTime && time < *lastTime))
            throw std::range_error("Native event clock must be finite and monotonic");
        lastTime = time;
        return time;
    }
    void promoteTimers() {
        const auto time = now();
        std::lock_guard lock(mailbox->mutex);
        while (!timers.empty() && timers.begin()->first.first <= time) {
            const auto timer = timers.begin();
            enqueueLocked(*mailbox, Envelope{timer->second, 0, {}});
            callbacks.at(timer->second).timer.reset();
            timers.erase(timer);
        }
        if (!mailbox->ready.empty()) mailbox->changed.notify_all();
    }
};
NativeEventLoop::NativeEventLoop(Clock clock) : state_(std::make_unique<State>(std::move(clock))) {}
NativeEventLoop::~NativeEventLoop() {
    if (!isOwnerThread() || state_->running) std::terminate();
    state_->closing = true;
    const auto mailbox = state_->mailbox;
    {
        std::lock_guard lock(mailbox->mutex);
        mailbox->open = false;
        mailbox->receivers.clear(); mailbox->pending.clear(); mailbox->ready.clear();
    }
    mailbox->changed.notify_all();
    state_->timers.clear();
    // JS roots can finalize native payloads. Detach nodes before destroying them,
    // with all state still alive and owner APIs already rejecting reentry.
    while (!state_->callbacks.empty()) {
        auto retired = state_->callbacks.extract(state_->callbacks.begin());
    }
    while (!state_->receivers.empty()) {
        auto retired = state_->receivers.extract(state_->receivers.begin());
    }
}
bool NativeEventLoop::isOwnerThread() const noexcept {
    return NativeEventLoop_detail::currentThreadIdentity() == state_->owner;
}
void NativeEventLoop::requireOwnerThread() const {
    if (!isOwnerThread()) throw std::logic_error("Native event loop requires its owning thread");
    if (state_->closing) throw std::logic_error("Native event loop is closing");
}
NativeEventLoop::Sender NativeEventLoop::sender() const { requireOwnerThread(); return Sender(state_->mailbox); }
Id NativeEventLoop::Sender::post(Id receiver, NativeEventData data) const {
    const auto mailbox = mailbox_.lock();
    if (!mailbox) return 0;
    std::lock_guard lock(mailbox->mutex);
    if (!mailbox->open || !mailbox->receivers.contains(receiver)) return 0;
    const auto task = NativeEventLoop_detail::takeId(nextId);
    enqueueLocked(*mailbox, Envelope{task, receiver, std::move(data)});
    mailbox->changed.notify_all();
    return task;
}
void NativeEventLoop::Sender::wake() const {
    if (const auto mailbox = mailbox_.lock()) {
        {
            std::lock_guard lock(mailbox->mutex);
            ++mailbox->wakeRevision;
        }
        mailbox->changed.notify_all();
    }
}
Id NativeEventLoop::postTask(const JSValue& callback, const JSValue& receiver,
                             const std::vector<JSValue>& arguments) {
    requireOwnerThread();
    validateInputs(callback, receiver, arguments);
    if (!callback.isCallable()) throw JSTypeError("Native task callback must be callable");
    state_->promoteTimers();
    const auto task = NativeEventLoop_detail::takeId(nextId);
    state_->callbacks.emplace(task, State::Callback{callback, receiver, arguments, {}});
    try {
        std::lock_guard lock(state_->mailbox->mutex);
        enqueueLocked(*state_->mailbox, Envelope{task, 0, {}});
    } catch (...) { auto retired = state_->callbacks.extract(task); throw; }
    state_->mailbox->changed.notify_all();
    return task;
}
Id NativeEventLoop::setTimeout(const JSValue& callback, double delayMilliseconds,
                              const JSValue& receiver, const std::vector<JSValue>& arguments) {
    requireOwnerThread();
    if (!std::isfinite(delayMilliseconds) || delayMilliseconds < 0)
        throw std::invalid_argument("Native timer delay must be finite and nonnegative");
    validateInputs(callback, receiver, arguments);
    if (!callback.isCallable()) throw JSTypeError("Native timer callback must be callable");
    if (delayMilliseconds == 0) return postTask(callback, receiver, arguments);
    state_->promoteTimers();
    const auto deadline = *state_->lastTime + delayMilliseconds;
    if (!std::isfinite(deadline)) throw std::overflow_error("Native timer deadline overflow");
    const auto task = NativeEventLoop_detail::takeId(nextId);
    const auto key = std::pair{deadline, task};
    state_->callbacks.emplace(task, State::Callback{callback, receiver, arguments, key});
    try { state_->timers.emplace(key, task); }
    catch (...) { auto retired = state_->callbacks.extract(task); throw; }
    state_->mailbox->changed.notify_all();
    return task;
}
bool NativeEventLoop::cancelTask(Id task) {
    requireOwnerThread();
    bool canceled = false;
    if (auto callback = state_->callbacks.find(task); callback != state_->callbacks.end()) {
        if (callback->second.timer) state_->timers.erase(*callback->second.timer);
        canceled = true;
    }
    {
        std::lock_guard lock(state_->mailbox->mutex);
        canceled = state_->mailbox->pending.erase(task) != 0 || canceled;
        std::erase_if(state_->mailbox->ready, [task](const auto& envelope) { return envelope.task == task; });
    }
    auto retired = state_->callbacks.extract(task);
    if (canceled) state_->mailbox->changed.notify_all();
    // The final root may reenter or destroy this loop. No state access follows
    // destruction of the detached callback at function exit.
    return canceled;
}
Id NativeEventLoop::registerReceiver(NativeHandler handler, const JSValue& receiver,
                                    const std::vector<JSValue>& captures) {
    requireOwnerThread();
    if (!handler) throw std::invalid_argument("Native receiver handler must be present");
    validateInputs({}, receiver, captures);
    const auto id = NativeEventLoop_detail::takeId(nextId);
    state_->receivers.emplace(id, State::Receiver{handler, receiver, captures});
    try {
        std::lock_guard lock(state_->mailbox->mutex);
        state_->mailbox->receivers.insert(id);
    } catch (...) { auto retired = state_->receivers.extract(id); throw; }
    return id;
}
bool NativeEventLoop::unregisterReceiver(Id receiver) {
    requireOwnerThread();
    {
        std::lock_guard lock(state_->mailbox->mutex);
        if (!state_->mailbox->receivers.erase(receiver)) return false;
        std::erase_if(state_->mailbox->ready, [&](const auto& envelope) {
            if (envelope.receiver != receiver) return false;
            state_->mailbox->pending.erase(envelope.task); return true;
        });
    }
    auto retired = state_->receivers.extract(receiver);
    state_->mailbox->changed.notify_all();
    return true;
}
NativeEventLoop::RunResult NativeEventLoop::runOneTask() {
    requireOwnerThread();
    RunResult result;
    if (pumping || isDrainingMicrotasks() || isDrainingFinalizationJobs()) return result;
    struct Guard {
        State& state;
        explicit Guard(State& state) : state(state) { pumping = true; state.running = true; }
        ~Guard() { state.running = false; pumping = false; }
    } guard(*state_);
    const auto checkpoint = [&](ErrorPhase phase) {
        try { drainMicrotasks(); }
        catch (...) { result.failures.push_back({phase, std::current_exception()}); }
    };
    checkpoint(ErrorPhase::BeforeTask);
    state_->promoteTimers();
    Envelope envelope;
    {
        std::lock_guard lock(state_->mailbox->mutex);
        if (!state_->mailbox->ready.empty()) {
            envelope = std::move(state_->mailbox->ready.front()); state_->mailbox->ready.pop_front();
            state_->mailbox->pending.erase(envelope.task);
        }
    }
    if (envelope.task == 0) {
        // Cleanup is its own host task source, with a checkpoint after the job.
        // It never runs inline in GC, a property lookup, or a native callback.
        if (hasPendingFinalizationJobs()) {
            result.task = NativeEventLoop_detail::takeId(nextId);
            try {
                std::vector<FinalizationJobFailure> failures;
                if (drainFinalizationJobs(1, &failures) == 0) result.task = 0;
                for (auto& failure : failures)
                    result.failures.push_back({failure.microtaskCheckpoint ? ErrorPhase::AfterTask : ErrorPhase::Task,
                                               std::move(failure.error)});
            }
            catch (...) { result.failures.push_back({ErrorPhase::Task, std::current_exception()}); }
        }
        return result;
    }
    result.task = envelope.task;
    try {
        if (envelope.receiver == 0) {
            auto callback = std::move(state_->callbacks.at(envelope.task));
            state_->callbacks.erase(envelope.task);
            callback.function.call(callback.receiver, callback.arguments);
        } else {
            // A local copy remains rooted when the callback unregisters itself.
            const auto receiver = state_->receivers.at(envelope.receiver);
            receiver.handler(receiver.receiver, envelope.data, receiver.captures);
        }
    } catch (...) { result.failures.push_back({ErrorPhase::Task, std::current_exception()}); }
    checkpoint(ErrorPhase::AfterTask);
    return result;
}
bool NativeEventLoop::hasReadyTasks() {
    requireOwnerThread(); state_->promoteTimers();
    const bool finalization = hasPendingFinalizationJobs();
    std::lock_guard lock(state_->mailbox->mutex); return !state_->mailbox->ready.empty() || finalization;
}
bool NativeEventLoop::hasPendingTasks() const {
    requireOwnerThread();
    if (hasPendingMicrotasks() || hasPendingFinalizationJobs()) return true;
    if (!state_->timers.empty()) return true;
    std::lock_guard lock(state_->mailbox->mutex); return !state_->mailbox->ready.empty();
}
bool NativeEventLoop::waitForWork(std::chrono::milliseconds maximumWait) {
    requireOwnerThread();
    if (maximumWait.count() < 0) throw std::invalid_argument("Native wait duration must be nonnegative");
    if (pumping || isDrainingMicrotasks() || isDrainingFinalizationJobs()) return false;
    if (hasPendingMicrotasks() || hasPendingFinalizationJobs()) return true;
    // Snapshot before calling an injected clock. A wake between that read and
    // entering the wait must still trigger a fresh timer promotion.
    std::uint64_t observedWake;
    {
        std::lock_guard lock(state_->mailbox->mutex);
        observedWake = state_->mailbox->wakeRevision;
    }
    state_->promoteTimers();
    if (hasPendingMicrotasks() || hasPendingFinalizationJobs()) return true;
    auto duration = std::chrono::duration<double, std::milli>(maximumWait);
    if (!state_->timers.empty()) duration = std::min(duration,
        std::chrono::duration<double, std::milli>(state_->timers.begin()->first.first - *state_->lastTime));
    {
        std::unique_lock lock(state_->mailbox->mutex);
        if (!state_->mailbox->ready.empty()) return true;
        if (duration.count() == 0) return false;
        state_->mailbox->changed.wait_for(lock, duration, [&] {
            return !state_->mailbox->open || !state_->mailbox->ready.empty() ||
                   state_->mailbox->wakeRevision != observedWake;
        });
    }
    state_->promoteTimers();
    std::lock_guard lock(state_->mailbox->mutex);
    return !state_->mailbox->ready.empty() || hasPendingMicrotasks() || hasPendingFinalizationJobs();
}
