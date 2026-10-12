#include "core/NativeEventLoop.h"
#include "core/JSPromise.h"
#include <iostream>
#include <stdexcept>

namespace {
using Args = std::vector<JSValue>;
void log(JSValue events, JSValue value) { events.get("push").call(events, {value}); }
JSValue callback(JSValue events, const char* value) {
    return JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
        log(captures[0], captures[1]); return {};
    }, {events, value});
}
JSValue reference(NativeEventLoop& loop) {
    auto pointer = std::make_shared<NativeEventLoop*>(&loop);
    return JSValue::hostObject(pointer, pointer.get(), "eventloop.parity.reference");
}
NativeEventLoop& unwrap(JSValue value) {
    return **std::static_pointer_cast<NativeEventLoop*>(value.hostPayload("eventloop.parity.reference"));
}
JSValue scenario(unsigned kind) {
    NativeEventLoop loop; auto events = JSValue::array(), errors = JSValue::array(), state = JSValue::object(nullptr);
    auto owner = reference(loop); log(events, "sync");
    if (kind == 0) {
        loop.setTimeout(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
            log(captures[0], "timer:A"); queueMicrotask(callback(captures[0], "micro:A"));
            auto promise = resolveNativePromise(); promise.get("then").call(promise, {callback(captures[0], "promise:A")}); return {};
        }, {events}));
        loop.setTimeout(callback(events, "timer:B"));
        queueMicrotask(callback(events, "micro:script"));
        auto promise = resolveNativePromise(); promise.get("then").call(promise, {callback(events, "promise:script")});
    } else if (kind == 1) {
        loop.setTimeout(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
            log(captures[0], "timer:A"); unwrap(captures[1]).setTimeout(callback(captures[0], "timer:C"));
            queueMicrotask(callback(captures[0], "micro:A")); return {};
        }, {events, owner}));
        loop.setTimeout(callback(events, "timer:B"));
    } else if (kind == 2) {
        loop.setTimeout(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
            log(captures[0], "timer:A"); unwrap(captures[1]).cancelTask(static_cast<std::uint64_t>(captures[2].get("cancel").number()));
            queueMicrotask(callback(captures[0], "micro:A")); return {};
        }, {events, owner, state}));
        state.set("cancel", loop.setTimeout(callback(events, "canceled")));
        loop.setTimeout(callback(events, "timer:C"));
    } else if (kind == 3) {
        const auto deferred = createNativePromiseCapability();
        deferred.promise.get("then").call(deferred.promise, {JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
            log(captures[0], "promise"); queueMicrotask(callback(captures[0], "micro:promise")); return {};
        }, {events})});
        loop.setTimeout(JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
            log(captures[0], "timer:A"); captures[1].call(); queueMicrotask(callback(captures[0], "micro:A")); return {};
        }, {events, deferred.resolve}));
        loop.setTimeout(callback(events, "timer:B"));
    }
    log(events, "return");
    while (true) {
        const auto result = loop.runOneTask();
        for (const auto& failure : result.failures) {
            try { std::rethrow_exception(failure.error); }
            catch (const JSThrownValue& thrown) { log(errors, thrown.value); }
        }
        if (!result.ranTask()) break;
    }
    auto result = JSValue::object(); result.set("events", events); result.set("errors", errors); return result;
}
}
int main() {
    auto results = JSValue::array(); for (unsigned kind = 0; kind < 4; ++kind) log(results, scenario(kind));
    std::cout << results.toJSON()->dump() << '\n';
}
