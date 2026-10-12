#include "catch.hpp"
#include "core/NativeEventLoop.h"
#include "core/JSJobs.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace {
using ReentryArgs = std::vector<JSValue>;
struct NativeFinalizer {
    std::function<void()> action;
    ~NativeFinalizer() { action(); }
};
JSValue finalizingPayload(std::function<void()> action) {
    auto owner = std::make_shared<NativeFinalizer>(); owner->action = std::move(action);
    return JSValue::hostObject(owner, owner.get(), "eventloop.reentry.finalizer");
}
JSValue finalizingCallback(std::function<void()> action) {
    return JSValue::function([](const JSValue&, const ReentryArgs&, const ReentryArgs&)->JSValue { return {}; },
                             {finalizingPayload(std::move(action))});
}
JSValue reentryNoop() {
    return JSValue::function([](const JSValue&, const ReentryArgs&, const ReentryArgs&)->JSValue { return {}; });
}
void reentryNativeNoop(const JSValue&, const NativeEventData&, const ReentryArgs&) {}
// No JS handles or exceptions cross the worker boundary. Fresh thread runtimes
// avoid constructor roots retained by unrelated tests in the main runtime.
template<class Work> std::string freshReentryRuntime(Work work) {
    std::string error;
    std::thread worker([&] {
        try { work(); }
        catch (const std::exception& exception) { error = exception.what(); }
        catch (...) { error = "Unexpected native runtime failure"; }
    });
    worker.join(); return error;
}
struct ReentryObservation {
    NativeEventLoop* loop = nullptr;
    std::unique_ptr<NativeEventLoop>* loopOwner = nullptr;
    NativeEventLoop::Sender sender;
    NativeEventLoop::Id receiver = 0, replacement = 0, nestedTask = 0;
    unsigned destroyed = 0, failures = 0, rejected = 0, unexpected = 0;
    bool active = true, late = false, threw = false, canceled = false;
    bool immediate = false, gone = false, absent = false, pending = true, owner = false;
    bool promoted = true;
};
struct DeactivateFinalizers {
    std::shared_ptr<ReentryObservation> state;
    ~DeactivateFinalizers() {
        state->active = false; state->loop = nullptr; state->loopOwner = nullptr;
    }
};
bool liveFinalizer(const std::shared_ptr<ReentryObservation>& state) {
    ++state->destroyed;
    if (state->active) return true;
    state->late = true; return false;
}
}
TEST_CASE("Native cancellation retires queue and timer before releasing final callback roots", "[runtime][eventloop][gc][reentry]") {
    auto state = std::make_shared<ReentryObservation>(); unsigned mode = 0;
    SECTION("queued task") { mode = 0; }
    SECTION("future timer") { mode = 1; }
    SECTION("promoted timer") { mode = 2; }
    const auto error = freshReentryRuntime([&] {
        double now = 0; NativeEventLoop loop([&] { return now; }); state->loop = &loop;
        DeactivateFinalizers guard{state};
        const auto finalizer = [state] {
            if (!liveFinalizer(state)) return;
            try {
                const auto nested = state->loop->runOneTask();
                state->nestedTask = nested.task; state->failures = static_cast<unsigned>(nested.failures.size());
            } catch (...) { state->threw = true; }
        };
        const auto id = mode == 0 ? loop.postTask(finalizingCallback(finalizer)) :
                                   loop.setTimeout(finalizingCallback(finalizer), 10);
        if (mode != 0) now = 10;
        if (mode == 2) state->promoted = loop.hasReadyTasks();
        state->canceled = loop.cancelTask(id); state->immediate = state->destroyed == 1;
        state->pending = loop.hasPendingTasks(); state->absent = !loop.cancelTask(id);
    });
    REQUIRE(error.empty()); REQUIRE(state->canceled); REQUIRE(state->immediate); REQUIRE(state->destroyed == 1);
    REQUIRE_FALSE(state->threw); REQUIRE_FALSE(state->late); REQUIRE(state->nestedTask == 0); REQUIRE(state->failures == 0);
    REQUIRE_FALSE(state->pending); REQUIRE(state->absent); REQUIRE(state->promoted);
}
TEST_CASE("Native callback finalization may destroy its idle loop after cancellation", "[runtime][eventloop][gc][reentry]") {
    auto state = std::make_shared<ReentryObservation>();
    const auto error = freshReentryRuntime([&] {
        auto loop = std::make_unique<NativeEventLoop>(); state->loopOwner = &loop;
        DeactivateFinalizers guard{state};
        const auto id = loop->postTask(finalizingCallback([state] {
            if (liveFinalizer(state)) state->loopOwner->reset();
        }));
        state->canceled = loop->cancelTask(id); state->gone = !loop;
    });
    REQUIRE(error.empty()); REQUIRE(state->canceled); REQUIRE(state->destroyed == 1); REQUIRE(state->gone);
    REQUIRE_FALSE(state->late);
}
TEST_CASE("Native receiver roots are released outside receiver container mutation", "[runtime][eventloop][gc][reentry]") {
    auto state = std::make_shared<ReentryObservation>();
    const auto error = freshReentryRuntime([&] {
        NativeEventLoop loop; state->loop = &loop; state->sender = loop.sender();
        DeactivateFinalizers guard{state};
        state->receiver = loop.registerReceiver(reentryNativeNoop, {}, {finalizingPayload([state] {
            if (!liveFinalizer(state)) return;
            try {
                auto& loop = *state->loop;
                state->absent = !loop.unregisterReceiver(state->receiver) && state->sender.post(state->receiver, {}) == 0;
                for (unsigned index = 0; index < 1024; ++index)
                    state->replacement = loop.registerReceiver(reentryNativeNoop);
                const auto nested = loop.runOneTask(); state->nestedTask = nested.task;
                state->failures = static_cast<unsigned>(nested.failures.size());
            } catch (...) { state->threw = true; }
        })});
        if (!state->sender.post(state->receiver, {}) || !state->sender.post(state->receiver, {}))
            throw std::runtime_error("Failed to queue native receiver deliveries");
        state->canceled = loop.unregisterReceiver(state->receiver); state->immediate = state->destroyed == 1;
        state->pending = loop.hasPendingTasks();
        if (state->sender.post(state->receiver, {}) != 0 || !loop.unregisterReceiver(state->replacement))
            throw std::runtime_error("Unexpected receiver identity after finalization");
    });
    REQUIRE(error.empty()); REQUIRE(state->canceled); REQUIRE(state->immediate); REQUIRE(state->destroyed == 1);
    REQUIRE_FALSE(state->threw); REQUIRE_FALSE(state->late); REQUIRE(state->absent); REQUIRE(state->replacement > state->receiver);
    REQUIRE(state->nestedTask == 0); REQUIRE(state->failures == 0); REQUIRE_FALSE(state->pending);
}
TEST_CASE("Native receiver finalization may destroy its idle loop after unregistering", "[runtime][eventloop][gc][reentry]") {
    auto state = std::make_shared<ReentryObservation>();
    const auto error = freshReentryRuntime([&] {
        auto loop = std::make_unique<NativeEventLoop>(); state->loopOwner = &loop;
        DeactivateFinalizers guard{state};
        const auto id = loop->registerReceiver(reentryNativeNoop, {}, {finalizingPayload([state] {
            if (liveFinalizer(state)) state->loopOwner->reset();
        })});
        state->canceled = loop->unregisterReceiver(id); state->gone = !loop;
    });
    REQUIRE(error.empty()); REQUIRE(state->canceled); REQUIRE(state->destroyed == 1); REQUIRE(state->gone);
    REQUIRE_FALSE(state->late);
}
TEST_CASE("Native loop closes owner APIs before any callback receiver or clock finalizer", "[runtime][eventloop][gc][reentry]") {
    auto state = std::make_shared<ReentryObservation>(); unsigned mode = 0;
    SECTION("callback roots with a live timer") { mode = 0; }
    SECTION("receiver roots with queued native delivery") { mode = 1; }
    SECTION("clock native capture after all scheduling containers are destroyed") { mode = 2; }
    const auto error = freshReentryRuntime([&] {
        DeactivateFinalizers guard{state};
        const auto finalize = [state] {
            if (!liveFinalizer(state)) return;
            auto& loop = *state->loop; state->owner = loop.isOwnerThread();
            const auto check = [&](auto action) {
                try { action(); ++state->unexpected; }
                catch (const std::logic_error& error) {
                    if (std::string(error.what()) == "Native event loop is closing") ++state->rejected;
                    else ++state->unexpected;
                } catch (...) { ++state->unexpected; }
            };
            check([&] { loop.sender(); }); check([&] { loop.postTask(17); });
            check([&] { loop.setTimeout(17, -1); }); check([&] { loop.cancelTask(0); });
            check([&] { loop.registerReceiver(nullptr); }); check([&] { loop.unregisterReceiver(0); });
            check([&] { loop.runOneTask(); }); check([&] { loop.hasReadyTasks(); });
            check([&] { loop.hasPendingTasks(); }); check([&] { loop.waitForWork(std::chrono::milliseconds(-1)); });
            if (state->sender.post(state->receiver, {}) != 0) ++state->unexpected;
            state->sender.wake();
        };
        if (mode == 0) {
            NativeEventLoop loop([] { return 0.; }); state->loop = &loop; state->sender = loop.sender();
            loop.postTask(finalizingCallback(finalize)); loop.setTimeout(reentryNoop(), 1000);
        } else if (mode == 1) {
            NativeEventLoop loop; state->loop = &loop; state->sender = loop.sender();
            state->receiver = loop.registerReceiver(reentryNativeNoop, {}, {finalizingPayload(finalize)});
            if (state->sender.post(state->receiver, {}) == 0) throw std::runtime_error("Failed native receiver post");
        } else {
            auto capture = std::make_shared<NativeFinalizer>(); capture->action = finalize;
            NativeEventLoop loop([capture] { return 0.; }); state->loop = &loop; state->sender = loop.sender();
            capture.reset();
        }
    });
    REQUIRE(error.empty()); REQUIRE(state->destroyed == 1); REQUIRE_FALSE(state->late); REQUIRE(state->owner);
    REQUIRE(state->rejected == 10); REQUIRE(state->unexpected == 0); REQUIRE(state->sender.post(state->receiver, {}) == 0);
}
