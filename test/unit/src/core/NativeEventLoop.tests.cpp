#include "catch.hpp"
#include "core/NativeEventLoop.h"
#include "core/NativeEventLoopDetail.h"
#include "core/JSPromise.h"
#include "core/JSJobs.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <thread>

namespace {
using Args = std::vector<JSValue>;
void append(JSValue events, const JSValue& value) { events.get("push").call(events, {value}); }
JSValue logTask(JSValue events, JSValue value) {
    return JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
        append(captures[0], captures[1]); return {};
    }, {events, value});
}
std::string trace(JSValue events) { return events.get("join").call(events, {","}).string(); }
JSValue loopReference(NativeEventLoop& loop) {
    auto pointer = std::make_shared<NativeEventLoop*>(&loop);
    return JSValue::hostObject(pointer, pointer.get(), "eventloop.test.reference");
}
NativeEventLoop& loopFrom(const JSValue& reference) {
    return **std::static_pointer_cast<NativeEventLoop*>(reference.hostPayload("eventloop.test.reference"));
}
void nativeLog(const JSValue& receiver, const NativeEventData& data, const Args&) { append(receiver, data.text); }
std::string thrownValue(const std::exception_ptr& error) {
    try { std::rethrow_exception(error); }
    catch (const JSThrownValue& thrown) { return thrown.value.string(); }
}
}
TEST_CASE("Native timer tasks preserve the cached loader microtask order", "[runtime][eventloop]") {
    NativeEventLoop loop; auto events = JSValue::array();
    loop.setTimeout(JSValue::function([](const JSValue& receiver, const Args& args, const Args& captures)->JSValue {
        append(captures[0], "cache");
        append(captures[0], receiver.get("name"));
        append(captures[0], args.at(0));
        queueMicrotask(logTask(captures[0], "micro:cache")); return {};
    }, {events}), 0, JSValue::fromJSON(JsonValue::Object{{"name", "receiver"}}), {"argument"});
    loop.postTask(logTask(events, "next"));
    queueMicrotask(logTask(events, "micro:script"));
    REQUIRE(trace(events).empty());
    const auto first = loop.runOneTask();
    REQUIRE(first.ranTask()); REQUIRE(first.failures.empty());
    REQUIRE(trace(events) == "micro:script,cache,receiver,argument,micro:cache");
    REQUIRE(loop.runOneTask().ranTask());
    REQUIRE(trace(events) == "micro:script,cache,receiver,argument,micro:cache,next");
    REQUIRE_FALSE(loop.runOneTask().ranTask());
    REQUIRE_FALSE(hasPendingMicrotasks());
}
TEST_CASE("Native delayed timers use injected time, deadline order, and cancellation", "[runtime][eventloop]") {
    double now = 100; NativeEventLoop loop([&] { return now; }); auto events = JSValue::array();
    const auto late = loop.setTimeout(logTask(events, "late"), 10);
    const auto early = loop.setTimeout(logTask(events, "early"), 5);
    const auto tie = loop.setTimeout(logTask(events, "tie"), 10);
    const auto canceled = loop.setTimeout(logTask(events, "canceled"), 7);
    REQUIRE(loop.cancelTask(canceled)); REQUIRE_FALSE(loop.cancelTask(canceled));
    REQUIRE(loop.hasPendingTasks()); REQUIRE_FALSE(loop.hasReadyTasks());
    now = 104; REQUIRE_FALSE(loop.runOneTask().ranTask());
    now = 105; REQUIRE(loop.runOneTask().task == early);
    REQUIRE(trace(events) == "early");
    now = 110; REQUIRE(loop.runOneTask().task == late); REQUIRE(loop.runOneTask().task == tie);
    REQUIRE(trace(events) == "early,late,tie"); REQUIRE_FALSE(loop.hasPendingTasks());
    const auto ready = loop.setTimeout(logTask(events, "ready"), 0);
    REQUIRE(loop.cancelTask(ready)); REQUIRE_FALSE(loop.hasReadyTasks());
    REQUIRE_FALSE(loop.cancelTask(late));
}
TEST_CASE("Native task reentry is deferred and consumed tasks cannot be canceled", "[runtime][eventloop]") {
    NativeEventLoop loop, other; auto events = JSValue::array(); auto task = JSValue::object(nullptr);
    const auto reference = loopReference(loop), second = loopReference(other);
    const auto id = loop.postTask(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
        auto& loop = loopFrom(captures[1]); auto& other = loopFrom(captures[2]);
        append(captures[0], "outer");
        append(captures[0], loop.cancelTask(static_cast<std::uint64_t>(captures[3].get("id").number())));
        append(captures[0], loop.runOneTask().ranTask()); append(captures[0], other.runOneTask().ranTask());
        append(captures[0], loop.waitForWork(std::chrono::milliseconds(1)));
        loop.postTask(logTask(captures[0], "nested"));
        queueMicrotask(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
            append(captures[0], "micro"); append(captures[0], loopFrom(captures[1]).runOneTask().ranTask()); return {};
        }, {captures[0], captures[1]})); return {};
    }, {events, reference, second, task}));
    task.set("id", id); other.postTask(logTask(events, "other"));
    loop.postTask(logTask(events, "next"));
    REQUIRE(loop.runOneTask().failures.empty());
    REQUIRE(trace(events) == "outer,false,false,false,false,micro,false");
    REQUIRE(loop.runOneTask().ranTask()); REQUIRE(loop.runOneTask().ranTask());
    REQUIRE(other.runOneTask().ranTask());
    REQUIRE(trace(events) == "outer,false,false,false,false,micro,false,next,nested,other");
}
TEST_CASE("Native event errors preserve each phase and permit later tasks", "[runtime][eventloop]") {
    NativeEventLoop loop; auto events = JSValue::array();
    queueMicrotask(JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { throw JSThrownValue("before"); }));
    loop.postTask(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
        append(captures[0], "task");
        queueMicrotask(JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { throw JSThrownValue("after"); }));
        queueMicrotask(logTask(captures[0], "micro:continued")); throw JSThrownValue("task");
    }, {events}));
    loop.postTask(logTask(events, "next"));
    const auto result = loop.runOneTask();
    REQUIRE(result.ranTask()); REQUIRE(result.failures.size() == 3);
    REQUIRE(result.failures[0].phase == NativeEventLoop::ErrorPhase::BeforeTask);
    REQUIRE(result.failures[1].phase == NativeEventLoop::ErrorPhase::Task);
    REQUIRE(result.failures[2].phase == NativeEventLoop::ErrorPhase::AfterTask);
    REQUIRE(thrownValue(result.failures[0].error) == "before");
    REQUIRE(thrownValue(result.failures[1].error) == "task");
    REQUIRE(thrownValue(result.failures[2].error) == "after");
    REQUIRE(trace(events) == "task,micro:continued");
    REQUIRE(loop.runOneTask().failures.empty()); REQUIRE(trace(events) == "task,micro:continued,next");
}
TEST_CASE("Native workers post only envelopes and cancellation discards queued delivery", "[runtime][eventloop][threads]") {
    NativeEventLoop loop; auto events = JSValue::array(); const auto sender = loop.sender();
    const auto receiver = loop.registerReceiver(nativeLog, events);
    std::vector<NativeEventLoop::Id> ids;
    std::atomic<int> wrongThreadRejected{0};
    std::thread worker([&] {
        for (const auto* text : {"one", "two", "three"}) {
            NativeEventData data; data.text = text; data.kind = 7; data.bytes = {std::byte{4}};
            ids.push_back(sender.post(receiver, std::move(data)));
        }
        try { loop.runOneTask(); } catch (const std::logic_error&) { ++wrongThreadRejected; }
        try { loop.cancelTask(ids[0]); } catch (const std::logic_error&) { ++wrongThreadRejected; }
        try { loop.hasReadyTasks(); } catch (const std::logic_error&) { ++wrongThreadRejected; }
        try { loop.hasPendingTasks(); } catch (const std::logic_error&) { ++wrongThreadRejected; }
        try { loop.waitForWork(std::chrono::milliseconds(0)); } catch (const std::logic_error&) { ++wrongThreadRejected; }
        try { loop.sender(); } catch (const std::logic_error&) { ++wrongThreadRejected; }
    }); worker.join();
    REQUIRE(wrongThreadRejected == 6); REQUIRE(trace(events).empty());
    REQUIRE(ids.size() == 3); REQUIRE(ids[0] < ids[1]); REQUIRE(ids[1] < ids[2]);
    REQUIRE(loop.cancelTask(ids[1])); REQUIRE_FALSE(loop.cancelTask(ids[1]));
    REQUIRE(loop.runOneTask().task == ids[0]); REQUIRE(loop.runOneTask().task == ids[2]);
    REQUIRE(trace(events) == "one,three");
    NativeEventData data; data.text = "discarded"; const auto discarded = sender.post(receiver, data);
    REQUIRE(discarded != 0); REQUIRE(loop.unregisterReceiver(receiver));
    REQUIRE_FALSE(loop.cancelTask(discarded)); REQUIRE_FALSE(loop.hasPendingTasks());
    REQUIRE(sender.post(receiver, data) == 0); REQUIRE_FALSE(loop.unregisterReceiver(receiver));
    const auto replacement = loop.registerReceiver(nativeLog, events); REQUIRE(replacement > receiver);
    REQUIRE(sender.post(receiver, data) == 0); REQUIRE(loop.unregisterReceiver(replacement));
}
TEST_CASE("Native receiver removal during delivery retains the current callback roots", "[runtime][eventloop][gc]") {
    NativeEventLoop loop; auto state = JSValue::object(nullptr); auto events = JSValue::array();
    auto reference = loopReference(loop); const auto sender = loop.sender();
    const auto id = loop.registerReceiver([](const JSValue&, const NativeEventData&, const Args& captures) {
        auto& loop = loopFrom(captures[1]);
        loop.unregisterReceiver(static_cast<std::uint64_t>(captures[2].get("id").number()));
        captures[0].collectGarbage(); append(captures[0], "retained");
        queueMicrotask(logTask(captures[0], "micro"));
    }, {}, {events, reference, state}); state.set("id", id);
    REQUIRE(sender.post(id, {}) != 0); REQUIRE(sender.post(id, {}) != 0);
    REQUIRE(loop.runOneTask().failures.empty()); REQUIRE(trace(events) == "retained,micro");
    REQUIRE_FALSE(loop.hasPendingTasks()); REQUIRE(sender.post(id, {}) == 0);
}
TEST_CASE("Native pending tasks retain captures until cancellation, execution, or loop destruction", "[runtime][eventloop][gc]") {
    auto anchor = JSValue::object(); std::weak_ptr<int> weak;
    SECTION("cancellation releases a pending timer") {
        NativeEventLoop loop; NativeEventLoop::Id id;
        {
            auto owner = std::make_shared<int>(42); weak = owner;
            auto payload = JSValue::hostObject(owner, owner.get(), "eventloop.test.payload");
            id = loop.setTimeout(JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { return {}; }, {payload}), 100);
        }
        anchor.collectGarbage(); REQUIRE_FALSE(weak.expired()); REQUIRE(loop.cancelTask(id));
        anchor.collectGarbage(); REQUIRE(weak.expired());
    }
    SECTION("execution retains captures through a checkpoint") {
        NativeEventLoop loop;
        {
            auto owner = std::make_shared<int>(42); weak = owner;
            auto payload = JSValue::hostObject(owner, owner.get(), "eventloop.test.payload");
            loop.postTask(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
                queueMicrotask(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
                    captures[0].collectGarbage();
                    if (*std::static_pointer_cast<int>(captures[0].hostPayload("eventloop.test.payload")) != 42)
                        throw std::runtime_error("Lost pending callback capture"); return {};
                }, captures)); return {};
            }, {payload}));
        }
        anchor.collectGarbage(); REQUIRE_FALSE(weak.expired()); REQUIRE(loop.runOneTask().failures.empty());
        anchor.collectGarbage(); REQUIRE(weak.expired());
    }
    SECTION("destroying the loop releases receiver captures and rejects late posts") {
        NativeEventLoop::Sender sender; NativeEventLoop::Id receiver;
        {
            NativeEventLoop loop; sender = loop.sender();
            auto owner = std::make_shared<int>(42); weak = owner;
            auto payload = JSValue::hostObject(owner, owner.get(), "eventloop.test.payload");
            receiver = loop.registerReceiver([](const JSValue&, const NativeEventData&, const Args&) {}, {}, {payload});
            REQUIRE(sender.post(receiver, {}) != 0);
        }
        anchor.collectGarbage(); REQUIRE(weak.expired());
        std::atomic<NativeEventLoop::Id> posted{99}; std::thread worker([&] { posted = sender.post(receiver, {}); sender.wake(); });
        worker.join(); REQUIRE(posted == 0);
        NativeEventLoop replacement; REQUIRE(replacement.registerReceiver(nativeLog, anchor) > receiver);
        REQUIRE(sender.post(receiver, {}) == 0);
    }
}
TEST_CASE("Native event IDs never wrap and delay and clock inputs are explicitly validated", "[runtime][eventloop]") {
    std::atomic<std::uint64_t> ids{std::numeric_limits<std::uint64_t>::max() - 2};
    REQUIRE(NativeEventLoop_detail::takeId(ids) == std::numeric_limits<std::uint64_t>::max() - 2);
    REQUIRE(NativeEventLoop_detail::takeId(ids) == std::numeric_limits<std::uint64_t>::max() - 1);
    REQUIRE_THROWS_AS(NativeEventLoop_detail::takeId(ids), std::overflow_error);
    REQUIRE_THROWS_AS(NativeEventLoop_detail::takeId(ids), std::overflow_error);
    std::atomic<std::uint64_t> invalid{0}; REQUIRE_THROWS_AS(NativeEventLoop_detail::takeId(invalid), std::overflow_error);
    double now = 10; NativeEventLoop loop([&] { return now; }); auto callback = logTask(JSValue::array(), "task");
    REQUIRE_THROWS_AS(loop.setTimeout(callback, -1), std::invalid_argument);
    REQUIRE_THROWS_AS(loop.setTimeout(callback, std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
    REQUIRE_THROWS_AS(loop.setTimeout(callback, std::numeric_limits<double>::infinity()), std::invalid_argument);
    REQUIRE_THROWS_AS(loop.postTask(17), JSTypeError);
    REQUIRE_THROWS_AS(loop.registerReceiver(nullptr), std::invalid_argument);
    REQUIRE_THROWS_AS(loop.waitForWork(std::chrono::milliseconds(-1)), std::invalid_argument);
    REQUIRE_FALSE(loop.hasReadyTasks()); now = 9; REQUIRE_THROWS_AS(loop.hasReadyTasks(), std::range_error);
    now = std::numeric_limits<double>::quiet_NaN(); REQUIRE_THROWS_AS(loop.hasReadyTasks(), std::range_error);
    now = std::numeric_limits<double>::infinity(); REQUIRE_THROWS_AS(loop.hasReadyTasks(), std::range_error);
    now = std::numeric_limits<double>::max(); REQUIRE_THROWS_AS(loop.setTimeout(callback, now), std::overflow_error);
}
TEST_CASE("Native loop thread identities never reuse exited thread identities", "[runtime][eventloop][threads]") {
    NativeEventLoop owner; std::uint64_t previous = 0;
    for (unsigned index = 0; index < 8; ++index) {
        std::uint64_t before = 99, assigned = 0, stable = 0;
        bool ownerRejected = false, ownAccepted = false;
        std::thread worker([&] {
            before = NativeEventLoop_detail::currentThreadIdentity();
            ownerRejected = !owner.isOwnerThread();
            NativeEventLoop loop; assigned = NativeEventLoop_detail::currentThreadIdentity();
            NativeEventLoop second; stable = NativeEventLoop_detail::currentThreadIdentity();
            ownAccepted = loop.isOwnerThread() && second.isOwnerThread();
            ownerRejected = ownerRejected && !owner.isOwnerThread();
        }); worker.join();
        REQUIRE(before == 0); REQUIRE(assigned > previous); REQUIRE(stable == assigned);
        REQUIRE(ownerRejected); REQUIRE(ownAccepted); previous = assigned;
    }
    REQUIRE(owner.isOwnerThread());
}
TEST_CASE("Native loops reject foreign managed inputs before copying but accept native primitives", "[runtime][eventloop][threads]") {
    double now = 0; unsigned clockReads = 0;
    NativeEventLoop loop([&] { ++clockReads; return now; }); auto events = JSValue::array();
    const auto callback = JSValue::function([](const JSValue& receiver, const Args& args, const Args& captures)->JSValue {
        append(captures[0], receiver); for (const auto& value : args) append(captures[0], value); return {};
    }, {events});
    struct Foreign {
        JSValue callable = JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { return {}; });
        JSValue object = JSValue::object();
        Args references{object};
        JSValue primitive = "native";
        Args primitives{17, "argument"};
    };
    std::mutex mutex; std::condition_variable changed; Foreign* foreign = nullptr; bool finished = false;
    std::thread worker([&] {
        Foreign values;
        std::unique_lock lock(mutex); foreign = &values; changed.notify_all();
        changed.wait(lock, [&] { return finished; });
        // Managed handles and their heap are destroyed on the producing thread.
    });
    struct Join {
        std::thread& worker; std::mutex& mutex; std::condition_variable& changed; bool& finished;
        ~Join() { { std::lock_guard lock(mutex); finished = true; } changed.notify_all(); worker.join(); }
    } join{worker, mutex, changed, finished};
    { std::unique_lock lock(mutex); changed.wait(lock, [&] { return foreign != nullptr; }); }
    // No foreign JSValue copies or heap accesses occur in the caller. Only const
    // references to immutable handles are presented to the validation boundary.
    REQUIRE_THROWS_AS(loop.postTask(foreign->callable), JSTypeError);
    REQUIRE_THROWS_AS(loop.postTask(callback, foreign->object), JSTypeError);
    REQUIRE_THROWS_AS(loop.postTask(callback, {}, foreign->references), JSTypeError);
    REQUIRE_THROWS_AS(loop.setTimeout(foreign->callable, 0), JSTypeError);
    REQUIRE_THROWS_AS(loop.setTimeout(foreign->callable, 1), JSTypeError);
    REQUIRE_THROWS_AS(loop.setTimeout(callback, 1, foreign->object), JSTypeError);
    REQUIRE_THROWS_AS(loop.setTimeout(callback, 1, {}, foreign->references), JSTypeError);
    REQUIRE_THROWS_AS(loop.registerReceiver(nativeLog, foreign->object), JSTypeError);
    REQUIRE_THROWS_AS(loop.registerReceiver(nativeLog, {}, foreign->references), JSTypeError);
    REQUIRE(clockReads == 0); REQUIRE_FALSE(loop.hasPendingTasks());
    loop.postTask(callback, foreign->primitive, foreign->primitives);
    loop.setTimeout(callback, 1, foreign->primitive, foreign->primitives);
    const auto receiver = loop.registerReceiver([](const JSValue& receiver, const NativeEventData&, const Args& captures) {
        for (const auto& value : captures) append(receiver, value);
    }, events, foreign->primitives);
    REQUIRE(loop.sender().post(receiver, {}) != 0);
    REQUIRE(loop.runOneTask().failures.empty()); REQUIRE(loop.runOneTask().failures.empty());
    now = 1; REQUIRE(loop.runOneTask().failures.empty());
    REQUIRE(trace(events) == "native,17,argument,17,argument,native,17,argument");
    REQUIRE(loop.unregisterReceiver(receiver)); REQUIRE_FALSE(loop.hasPendingTasks());
}
TEST_CASE("Native host pumping observes standalone microtasks shared by all loops on the thread", "[runtime][eventloop]") {
    NativeEventLoop first, second; auto events = JSValue::array();
    queueMicrotask(logTask(events, "microtask"));
    REQUIRE(first.hasPendingTasks()); REQUIRE(second.hasPendingTasks());
    REQUIRE_FALSE(first.hasReadyTasks()); REQUIRE_FALSE(second.hasReadyTasks());
    REQUIRE(first.waitForWork(std::chrono::milliseconds(0)));
    REQUIRE(second.waitForWork(std::chrono::milliseconds(0))); REQUIRE(trace(events).empty());
    const auto microtask = second.runOneTask(); REQUIRE_FALSE(microtask.ranTask()); REQUIRE(microtask.failures.empty());
    REQUIRE(trace(events) == "microtask"); REQUIRE_FALSE(first.hasPendingTasks()); REQUIRE_FALSE(second.hasPendingTasks());
    const auto promise = resolveNativePromise(); promise.get("then").call(promise, {logTask(events, "promise")});
    REQUIRE(first.hasPendingTasks()); REQUIRE(second.hasPendingTasks());
    REQUIRE(first.waitForWork(std::chrono::milliseconds(50))); REQUIRE(trace(events) == "microtask");
    const auto reaction = first.runOneTask(); REQUIRE_FALSE(reaction.ranTask()); REQUIRE(reaction.failures.empty());
    REQUIRE(trace(events) == "microtask,promise"); REQUIRE_FALSE(second.hasPendingTasks());
    queueMicrotask(JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { throw JSThrownValue("standalone"); }));
    const auto failed = second.runOneTask(); REQUIRE_FALSE(failed.ranTask()); REQUIRE(failed.failures.size() == 1);
    REQUIRE(failed.failures[0].phase == NativeEventLoop::ErrorPhase::BeforeTask);
    REQUIRE(thrownValue(failed.failures[0].error) == "standalone"); REQUIRE_FALSE(first.hasPendingTasks());
}
TEST_CASE("Native waits are bounded and never execute managed callbacks or microtasks", "[runtime][eventloop][threads]") {
    NativeEventLoop loop; auto events = JSValue::array(); const auto sender = loop.sender();
    const auto receiver = loop.registerReceiver(nativeLog, events);
    REQUIRE_FALSE(loop.waitForWork(std::chrono::milliseconds(2))); REQUIRE(trace(events).empty());
    queueMicrotask(logTask(events, "micro"));
    REQUIRE(loop.waitForWork(std::chrono::milliseconds(0))); REQUIRE(trace(events).empty());
    REQUIRE(hasPendingMicrotasks());
    std::thread worker([&] { NativeEventData data; data.text = "native"; sender.post(receiver, std::move(data)); });
    worker.join(); const auto ready = loop.waitForWork(std::chrono::milliseconds(50));
    REQUIRE(ready);
    REQUIRE(trace(events).empty()); REQUIRE(loop.runOneTask().failures.empty()); REQUIRE(trace(events) == "micro,native");
    REQUIRE(loop.unregisterReceiver(receiver));
}
TEST_CASE("Concurrent native posters preserve each producer order and globally unique IDs", "[runtime][eventloop][threads]") {
    NativeEventLoop loop; auto events = JSValue::array(); const auto sender = loop.sender();
    const auto receiver = loop.registerReceiver([](const JSValue& events, const NativeEventData& data, const Args&) {
        auto value = JSValue::array(); value.set("0", data.token); value.set("1", data.code); append(events, value);
    }, events);
    std::vector<std::thread> workers; std::vector<std::vector<NativeEventLoop::Id>> ids(4);
    for (std::uint64_t producer = 0; producer < 4; ++producer) workers.emplace_back([&, producer] {
        for (std::int64_t index = 0; index < 100; ++index) {
            NativeEventData data; data.token = producer; data.code = index;
            ids[producer].push_back(sender.post(receiver, std::move(data)));
        }
    });
    for (auto& worker : workers) worker.join();
    std::vector<NativeEventLoop::Id> combined; for (const auto& group : ids) combined.insert(combined.end(), group.begin(), group.end());
    std::sort(combined.begin(), combined.end()); REQUIRE(combined.size() == 400);
    REQUIRE(std::adjacent_find(combined.begin(), combined.end()) == combined.end()); REQUIRE(combined.front() != 0);
    for (std::size_t index = 0; index < 400; ++index) REQUIRE(loop.runOneTask().failures.empty());
    REQUIRE(events.length() == 400); std::int64_t next[4] = {};
    for (std::uint32_t index = 0; index < events.length(); ++index) {
        const auto event = events.get(std::to_string(index)); const auto producer = static_cast<std::size_t>(event.get("0").number());
        REQUIRE(event.get("1").number() == next[producer]++);
    }
    for (const auto count : next) REQUIRE(count == 100);
    REQUIRE_FALSE(loop.hasPendingTasks()); REQUIRE(loop.unregisterReceiver(receiver));
}

namespace {
using ReentryArgs = std::vector<JSValue>;
JSValue reentryNoop() {
    return JSValue::function([](const JSValue&, const ReentryArgs&, const ReentryArgs&)->JSValue { return {}; });
}
}
TEST_CASE("Standalone microtask checkpoints reject native pumps and waits across loops", "[runtime][eventloop][reentry]") {
    struct MicroState {
        NativeEventLoop *first, *second;
        std::vector<std::string> events;
        bool nestedRan = true, secondRan = true, waited = true, inside = false;
    };
    NativeEventLoop first, second;
    auto state = std::make_shared<MicroState>(); state->first = &first; state->second = &second;
    const auto payload = JSValue::hostObject(state, state.get(), "eventloop.reentry.micro");
    first.postTask(JSValue::function([](const JSValue&, const ReentryArgs&, const ReentryArgs& captures)->JSValue {
        auto state = std::static_pointer_cast<MicroState>(captures[0].hostPayload("eventloop.reentry.micro"));
        state->events.push_back("task:first"); return {};
    }, {payload}));
    second.postTask(JSValue::function([](const JSValue&, const ReentryArgs&, const ReentryArgs& captures)->JSValue {
        auto state = std::static_pointer_cast<MicroState>(captures[0].hostPayload("eventloop.reentry.micro"));
        state->events.push_back("task:second"); return {};
    }, {payload}));
    queueMicrotask(JSValue::function([](const JSValue&, const ReentryArgs&, const ReentryArgs& captures)->JSValue {
        auto state = std::static_pointer_cast<MicroState>(captures[0].hostPayload("eventloop.reentry.micro"));
        state->inside = isDrainingMicrotasks(); state->events.push_back("micro:first");
        state->nestedRan = state->first->runOneTask().ranTask(); state->secondRan = state->second->runOneTask().ranTask();
        state->waited = state->first->waitForWork(std::chrono::milliseconds(0)); return {};
    }, {payload}));
    queueMicrotask(JSValue::function([](const JSValue&, const ReentryArgs&, const ReentryArgs& captures)->JSValue {
        auto state = std::static_pointer_cast<MicroState>(captures[0].hostPayload("eventloop.reentry.micro"));
        state->events.push_back("micro:second"); return {};
    }, {payload}));
    REQUIRE_FALSE(isDrainingMicrotasks()); REQUIRE(drainMicrotasks() == 2);
    REQUIRE(state->inside); REQUIRE_FALSE(state->nestedRan); REQUIRE_FALSE(state->secondRan); REQUIRE_FALSE(state->waited);
    REQUIRE_FALSE(isDrainingMicrotasks()); REQUIRE(first.runOneTask().failures.empty());
    REQUIRE(second.runOneTask().failures.empty());
    REQUIRE(state->events == std::vector<std::string>{"micro:first", "micro:second", "task:first", "task:second"});
}
TEST_CASE("Native wake before waiting promotes timers using fresh injected time", "[runtime][eventloop][threads]") {
    using namespace std::chrono;
    std::mutex mutex; std::condition_variable changed;
    bool pause = false, inside = false, awoke = false; double now = 0;
    NativeEventLoop loop([&] {
        std::unique_lock lock(mutex); const double snapshot = now;
        if (pause) {
            pause = false; inside = true; changed.notify_all();
            changed.wait(lock, [&] { return awoke; });
        }
        return snapshot;
    });
    const auto timer = loop.setTimeout(reentryNoop(), 10000); const auto sender = loop.sender(); pause = true;
    std::thread worker([&] {
        std::unique_lock lock(mutex); changed.wait(lock, [&] { return inside; });
        now = 10000; sender.wake(); awoke = true; changed.notify_all();
    });
    const auto start = steady_clock::now(); const auto ready = loop.waitForWork(milliseconds(1000));
    const auto elapsed = steady_clock::now() - start; worker.join();
    REQUIRE(ready); REQUIRE(elapsed < milliseconds(500)); REQUIRE(loop.runOneTask().task == timer);
}
