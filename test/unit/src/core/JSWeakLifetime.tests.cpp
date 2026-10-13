#include "catch.hpp"
#include "core/JSWeakRef.h"
#include "core/JSFinalizationRegistry.h"
#include "core/JSWeakMap.h"
#include "core/JSJobs.h"
#include <thread>
#include <tuple>
#include <condition_variable>
#include <mutex>

namespace {
using Args = std::vector<JSValue>;
JSValue noop(const JSValue&, const Args&, const Args&) { return {}; }
JSValue recorder(const JSValue& log) {
    return JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures)->JSValue {
        auto log = captures[0];
        log.get("push").call(log, {self.isUndefined(), args[0]});
        return {};
    }, {log});
}
struct JobBoundary {
    JobBoundary() { endJSJob(); drainFinalizationJobs(); }
    ~JobBoundary() { endJSJob(); try { drainFinalizationJobs(); } catch (...) {} }
};
struct Resurrect {
    JSValue field;
    JSValue* saved = nullptr;
    ~Resurrect() { if (saved) *saved = field; }
};
std::vector<JSNativeOwnerNode> ownership(const JSValue& self) {
    auto owner = std::static_pointer_cast<Resurrect>(self.hostPayload("weaklife.test.owner"));
    return {{owner, {&owner->field}, {}}};
}
}
TEST_CASE("Weak lifetime constructors and prototype methods preserve built-in metadata", "[runtime][weaklife]") {
    JobBoundary boundary;
    for (const auto& ctor : Args{nativeWeakRefConstructor(), nativeFinalizationRegistryConstructor()}) {
        REQUIRE(ctor.isConstructible()); REQUIRE(ctor.get("length").number() == 1);
        REQUIRE_THROWS_AS(ctor.call(), JSTypeError);
        const auto descriptor = ctor.getOwnPropertyDescriptor("prototype");
        REQUIRE(descriptor->writable == false); REQUIRE(descriptor->enumerable == false);
        REQUIRE(descriptor->configurable == false);
        REQUIRE(ctor.get("prototype").get("constructor").sameValue(ctor));
    }
    REQUIRE(nativeWeakRefConstructor().get("name").string() == "WeakRef");
    REQUIRE(nativeFinalizationRegistryConstructor().get("name").string() == "FinalizationRegistry");
    for (const auto& [prototype, name, arity] : std::vector<std::tuple<JSValue, const char*, unsigned>>{
            {nativeWeakRefConstructor().get("prototype"), "deref", 0},
            {nativeFinalizationRegistryConstructor().get("prototype"), "register", 2},
            {nativeFinalizationRegistryConstructor().get("prototype"), "unregister", 1}}) {
        const auto method = prototype.get(name);
        const auto descriptor = prototype.getOwnPropertyDescriptor(name);
        REQUIRE(method.get("name").string() == name); REQUIRE(method.get("length").number() == arity);
        REQUIRE_FALSE(method.isConstructible()); REQUIRE_FALSE(method.hasOwn("prototype"));
        REQUIRE(descriptor->writable == true); REQUIRE(descriptor->enumerable == false);
        REQUIRE(descriptor->configurable == true);
        for (const auto& receiver : Args{{}, nullptr, 1, false, "x", JSValue::object(), prototype})
            REQUIRE_THROWS_AS(method.call(receiver, {nullptr}), JSTypeError);
    }
    REQUIRE_FALSE(isNativeWeakRef(nativeWeakRefConstructor().get("prototype")));
    REQUIRE_FALSE(isNativeFinalizationRegistry(nativeFinalizationRegistryConstructor().get("prototype")));
}
TEST_CASE("Weak lifetime construction validates before allocation and preserves newTarget prototype fallback", "[runtime][weaklife]") {
    JobBoundary boundary;
    const auto weakCtor = nativeWeakRefConstructor(), registryCtor = nativeFinalizationRegistryConstructor();
    auto target = JSValue::constructor(noop, {}, "Target", 0, JSValue::ConstructorKind::Callable);
    target.set("prototype", 7);
    const auto count = target.heapObjectCount();
    REQUIRE_THROWS_AS(weakCtor.construct({1}, target), JSTypeError);
    REQUIRE_THROWS_AS(registryCtor.construct({1}, target), JSTypeError);
    REQUIRE(target.heapObjectCount() == count);
    auto ref = nativeWeakRefConstructor().construct({JSValue::object()}, target);
    REQUIRE(ref.getPrototype().sameValue(nativeWeakRefConstructor().get("prototype")));
    const auto custom = JSValue::object(); target.set("prototype", custom);
    ref = nativeWeakRefConstructor().construct({JSValue::array()}, target);
    REQUIRE(ref.getPrototype().sameValue(custom)); REQUIRE(isNativeWeakRef(ref));
    auto registry = nativeFinalizationRegistryConstructor().construct({JSValue::function(noop)}, target);
    REQUIRE(registry.getPrototype().sameValue(custom)); REQUIRE(isNativeFinalizationRegistry(registry));
    ref.set("deref", JSValue::function(noop)); REQUIRE(ref.get("deref").call(ref).isUndefined());
}
TEST_CASE("WeakRef keeps object array and function targets through a complete job", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object();
    for (unsigned kind = 0; kind < 3; ++kind) {
        auto target = kind == 0 ? JSValue::object() : kind == 1 ? JSValue::array() : JSValue::function(noop);
        const auto type = target.type(); auto ref = createNativeWeakRef(target); JSWeakValue weak(target);
        REQUIRE(JSWeakRef(ref).deref().sameValue(target)); target = {};
        witness.collectGarbage(); REQUIRE_FALSE(weak.expired());
        REQUIRE(JSWeakRef(ref).deref().type() == type);
        endJSJob(); witness.collectGarbage(); REQUIRE(weak.expired());
        REQUIRE(JSWeakRef(ref).deref().isUndefined()); REQUIRE(JSWeakRef(ref).deref().isUndefined());
    }
    for (const auto& primitive : Args{{}, nullptr, false, 0, "target"})
        REQUIRE_THROWS_AS(createNativeWeakRef(primitive), JSTypeError);
}
TEST_CASE("A successful deref keeps the target through all checkpoint microtasks", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), target = JSValue::object(), log = JSValue::array();
    auto ref = createNativeWeakRef(target); JSWeakValue weak(target); endJSJob();
    queueMicrotask(JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
        REQUIRE(JSWeakRef(captures[0]).deref().isObject());
        captures[1].collectGarbage(); auto log = captures[2]; log.get("push").call(log, {true}); return {};
    }, {ref, witness, log}));
    target = {};
    queueMicrotask(JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
        REQUIRE(JSWeakRef(captures[0]).deref().isObject()); captures[1].collectGarbage(); return {};
    }, {ref, witness}));
    REQUIRE(drainMicrotasks() == 2); REQUIRE(log.length() == 1);
    witness.collectGarbage(); REQUIRE(weak.expired());
    auto next = JSValue::object(); auto nextRef = createNativeWeakRef(next); JSWeakValue weakNext(next); next = {};
    queueMicrotask(JSValue::function(+[](const JSValue&, const Args&, const Args&)->JSValue { throw JSTypeError("job"); }));
    REQUIRE_THROWS_AS(drainMicrotasks(), JSTypeError);
    witness.collectGarbage(); REQUIRE(weakNext.expired()); REQUIRE(JSWeakRef(nextRef).deref().isUndefined());
}
TEST_CASE("FinalizationRegistry retains held values and callbacks but never targets or tokens", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), log = JSValue::array();
    auto callback = recorder(log), held = JSValue::object(), target = JSValue::object(), token = JSValue::object();
    auto registry = createNativeFinalizationRegistry(callback);
    JSWeakValue weakCallback(callback), weakHeld(held), weakTarget(target), weakToken(token);
    JSFinalizationRegistry(registry).registerTarget(target, held, token); callback = {}; held = {}; token = {};
    witness.collectGarbage();
    REQUIRE_FALSE(weakCallback.expired()); REQUIRE_FALSE(weakHeld.expired()); REQUIRE(weakToken.expired());
    REQUIRE_FALSE(hasPendingFinalizationJobs()); REQUIRE(log.length() == 0);
    target = {}; witness.collectGarbage();
    REQUIRE(weakTarget.expired()); REQUIRE_FALSE(weakHeld.expired()); REQUIRE(log.length() == 0);
    REQUIRE(hasPendingFinalizationJobs()); REQUIRE(drainMicrotasks() == 0); REQUIRE(log.length() == 0);
    REQUIRE(drainFinalizationJobs() == 1); REQUIRE(log.length() == 2); REQUIRE(log.get("0").toBoolean());
    REQUIRE(log.get("1").sameValue(*weakHeld.lock())); REQUIRE_FALSE(hasPendingFinalizationJobs());
    log.set("length", 0); witness.collectGarbage(); REQUIRE(weakHeld.expired()); REQUIRE_FALSE(weakCallback.expired());
    registry = {}; witness.collectGarbage(); REQUIRE(weakCallback.expired());
}
TEST_CASE("FinalizationRegistry validates without coercion and unregister removes every matching cell", "[runtime][weaklife]") {
    JobBoundary boundary; auto registry = createNativeFinalizationRegistry(JSValue::function(noop));
    JSFinalizationRegistry adapter(registry); auto target = JSValue::object(), token = JSValue::object();
    for (const auto& primitive : Args{{}, nullptr, false, 0, "target"}) {
        REQUIRE_THROWS_AS(adapter.registerTarget(primitive, 1), JSTypeError);
        REQUIRE_THROWS_AS(adapter.unregister(primitive), JSTypeError);
        if (!primitive.isUndefined()) REQUIRE_THROWS_AS(adapter.registerTarget(target, 1, primitive), JSTypeError);
    }
    REQUIRE_THROWS_AS(adapter.registerTarget(target, target), JSTypeError);
    target.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args&, const Args&)->JSValue { throw JSTypeError("coerced"); }));
    adapter.registerTarget(target, {}, token); adapter.registerTarget(target, 2, token);
    REQUIRE(adapter.unregister(token)); REQUIRE_FALSE(adapter.unregister(token));
    REQUIRE_FALSE(adapter.unregister(JSValue::object()));
    REQUIRE(registry.get("register").call(registry, {target, 3, target}).isUndefined());
    REQUIRE(registry.get("unregister").call(registry, {target}).toBoolean());
}
TEST_CASE("Unregister can cancel collected cells before queued cleanup", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), log = JSValue::array(), token = JSValue::object();
    auto registry = createNativeFinalizationRegistry(recorder(log));
    { auto target = JSValue::object(); JSFinalizationRegistry(registry).registerTarget(target, "held", token); }
    witness.collectGarbage(); REQUIRE(hasPendingFinalizationJobs());
    REQUIRE(JSFinalizationRegistry(registry).unregister(token));
    REQUIRE(drainFinalizationJobs() == 1); REQUIRE(log.length() == 0); REQUIRE_FALSE(hasPendingFinalizationJobs());
}
TEST_CASE("Registry to held-value cycles and weak queued jobs cannot keep registries alive", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), log = JSValue::array(); JSWeakValue weakRegistry, weakHeld, weakCallback;
    {
        auto callback = recorder(log), registry = createNativeFinalizationRegistry(callback), held = JSValue::object();
        held.set("registry", registry); auto target = JSValue::object();
        JSFinalizationRegistry(registry).registerTarget(target, held);
        weakRegistry = JSWeakValue(registry); weakHeld = JSWeakValue(held); weakCallback = JSWeakValue(callback);
        target = {}; witness.collectGarbage(); REQUIRE(hasPendingFinalizationJobs());
    }
    witness.collectGarbage(); REQUIRE(weakRegistry.expired()); REQUIRE(weakHeld.expired()); REQUIRE(weakCallback.expired());
    REQUIRE_FALSE(hasPendingFinalizationJobs()); REQUIRE(drainFinalizationJobs() == 0); REQUIRE(log.length() == 0);
}
TEST_CASE("A held value reaching its target prevents finalization until unregister", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), log = JSValue::array(), token = JSValue::object();
    auto registry = createNativeFinalizationRegistry(recorder(log)), target = JSValue::object(), held = JSValue::object();
    held.set("target", target); JSWeakValue weakTarget(target), weakHeld(held);
    JSFinalizationRegistry(registry).registerTarget(target, held, token); target = {}; held = {};
    witness.collectGarbage(); REQUIRE_FALSE(weakTarget.expired()); REQUIRE_FALSE(weakHeld.expired());
    REQUIRE_FALSE(hasPendingFinalizationJobs()); REQUIRE(JSFinalizationRegistry(registry).unregister(token));
    witness.collectGarbage(); REQUIRE(weakTarget.expired()); REQUIRE(weakHeld.expired()); REQUIRE(log.length() == 0);
}
TEST_CASE("Cleanup removes each cell before callback and tolerates unregister GC and job reentry", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), log = JSValue::array(), token = JSValue::object();
    auto state = JSValue::object();
    auto callback = JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures)->JSValue {
        auto state = captures[0], log = captures[1];
        REQUIRE(self.isUndefined()); log.get("push").call(log, {args[0]});
        REQUIRE(drainFinalizationJobs() == 0);
        auto registry = state.get("registry");
        REQUIRE(JSFinalizationRegistry(registry).unregister(captures[2]));
        captures[3].collectGarbage();
        queueMicrotask(JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
            auto log = captures[0]; log.get("push").call(log, {"micro"}); return {};
        }, {log}));
        return {};
    }, {state, log, token, witness});
    auto registry = createNativeFinalizationRegistry(callback); state.set("registry", registry);
    { auto a = JSValue::object(), b = JSValue::object(); JSFinalizationRegistry(registry).registerTarget(a, "a");
      JSFinalizationRegistry(registry).registerTarget(b, "b", token); }
    witness.collectGarbage(); REQUIRE(drainFinalizationJobs() == 1);
    REQUIRE(log.length() == 2); REQUIRE(log.get("0").string() == "a"); REQUIRE(log.get("1").string() == "micro");
}
TEST_CASE("Cleanup errors end the task and defer remaining cells to a later task", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), log = JSValue::array();
    auto callback = JSValue::function(+[](const JSValue&, const Args& args, const Args& captures)->JSValue {
        auto log = captures[0]; log.get("push").call(log, {args[0]});
        if (args[0].strictEquals("a")) throw JSTypeError("cleanup"); return {};
    }, {log});
    auto registry = createNativeFinalizationRegistry(callback);
    { auto a = JSValue::object(), b = JSValue::object();
      JSFinalizationRegistry(registry).registerTarget(a, "a"); JSFinalizationRegistry(registry).registerTarget(b, "b"); }
    witness.collectGarbage(); REQUIRE_THROWS_AS(drainFinalizationJobs(), JSTypeError);
    REQUIRE(log.length() == 1); REQUIRE(hasPendingFinalizationJobs());
    REQUIRE(drainFinalizationJobs() == 1); REQUIRE(log.length() == 2); REQUIRE_FALSE(hasPendingFinalizationJobs());
}
TEST_CASE("Native finalizer resurrection is re-marked before weak targets clear or cleanup queues", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), target = JSValue::object(), log = JSValue::array(); JSValue saved;
    auto owner = std::make_shared<Resurrect>(); owner->field = target; owner->saved = &saved;
    auto host = JSValue::hostObject(owner, owner.get(), "weaklife.test.owner");
    auto hooks = std::make_shared<JSHostObjectHooks>(); hooks->nativeOwnership = ownership; host.setHostHooks(hooks);
    auto ref = createNativeWeakRef(target), registry = createNativeFinalizationRegistry(recorder(log)); endJSJob();
    JSFinalizationRegistry(registry).registerTarget(target, 42);
    target = {}; host = {}; owner.reset(); witness.collectGarbage();
    REQUIRE(saved.isObject()); REQUIRE(JSWeakRef(ref).deref().sameValue(saved)); REQUIRE_FALSE(hasPendingFinalizationJobs());
    saved = {}; endJSJob(); witness.collectGarbage(); REQUIRE(JSWeakRef(ref).deref().isUndefined());
    REQUIRE(drainFinalizationJobs() == 1); REQUIRE(log.get("1").number() == 42);
}
TEST_CASE("WeakRef kept targets activate ephemeron values without strengthening their weak edges", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), target = JSValue::object(), value = JSValue::object();
    JSWeakMap map; map.set(target, value); auto ref = createNativeWeakRef(target);
    JSWeakValue weakTarget(target), weakValue(value); target = {}; value = {};
    witness.collectGarbage(); REQUIRE_FALSE(weakTarget.expired()); REQUIRE_FALSE(weakValue.expired());
    endJSJob(); witness.collectGarbage(); REQUIRE(weakTarget.expired()); REQUIRE(weakValue.expired());
    REQUIRE(JSWeakRef(ref).deref().isUndefined());
}
TEST_CASE("Weak lifetime constructor caches expire with their heap and retain prototype mutation within it", "[runtime][weaklife][gc]") {
    bool retained = false, expired = false, fresh = false;
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            JSWeakValue weakRef, weakRegistry;
            { auto witness = JSValue::object();
              { auto a = nativeWeakRefConstructor(), b = nativeFinalizationRegistryConstructor();
                a.get("prototype").set("marker", 1); b.get("prototype").set("marker", 2);
                weakRef = JSWeakValue(a); weakRegistry = JSWeakValue(b); }
              witness.collectGarbage();
              retained = nativeWeakRefConstructor().sameValue(*weakRef.lock()) &&
                nativeFinalizationRegistryConstructor().sameValue(*weakRegistry.lock()) &&
                nativeWeakRefConstructor().get("prototype").get("marker").strictEquals(1) &&
                nativeFinalizationRegistryConstructor().get("prototype").get("marker").strictEquals(2); }
            expired = weakRef.expired() && weakRegistry.expired();
            fresh = nativeWeakRefConstructor().get("prototype").get("marker").isUndefined() &&
                nativeFinalizationRegistryConstructor().get("prototype").get("marker").isUndefined();
        } catch (...) { error = std::current_exception(); }
    }); worker.join(); if (error) std::rethrow_exception(error);
    REQUIRE(retained); REQUIRE(expired); REQUIRE(fresh);
}
TEST_CASE("Discarding a WeakRef keeps its target only until the host job ends", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(); JSWeakValue weakTarget, weakRef;
    { auto target = JSValue::object(), ref = createNativeWeakRef(target);
      weakTarget = JSWeakValue(target); weakRef = JSWeakValue(ref); }
    witness.collectGarbage(); REQUIRE(weakRef.expired()); REQUIRE_FALSE(weakTarget.expired());
    endJSJob(); witness.collectGarbage(); REQUIRE(weakTarget.expired());
}
TEST_CASE("External native owners retain weak targets until their ownership ends", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), log = JSValue::array();
    auto owner = std::make_shared<Resurrect>();
    auto target = JSValue::hostObject(owner, owner.get(), "weaklife.test.owner");
    auto hooks = std::make_shared<JSHostObjectHooks>(); hooks->nativeOwnership = ownership; target.setHostHooks(hooks);
    auto ref = createNativeWeakRef(target), registry = createNativeFinalizationRegistry(recorder(log));
    JSFinalizationRegistry(registry).registerTarget(target, "native"); target = {}; endJSJob(); witness.collectGarbage();
    REQUIRE(JSWeakRef(ref).deref().isObject()); REQUIRE_FALSE(hasPendingFinalizationJobs());
    owner.reset(); endJSJob(); witness.collectGarbage(); REQUIRE(JSWeakRef(ref).deref().isUndefined());
    REQUIRE(drainFinalizationJobs() == 1); REQUIRE(log.get("1").strictEquals("native"));
}
TEST_CASE("Pending weak cleanup tasks do not own their heap", "[runtime][weaklife][gc]") {
    bool expired = false, queueEmpty = false;
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            JSWeakValue weakRegistry, weakHeld;
            { auto witness = JSValue::object(), held = JSValue::object();
              auto registry = createNativeFinalizationRegistry(JSValue::function(noop));
              { auto target = JSValue::object(); JSFinalizationRegistry(registry).registerTarget(target, held); }
              weakRegistry = JSWeakValue(registry); weakHeld = JSWeakValue(held);
              witness.collectGarbage(); REQUIRE(hasPendingFinalizationJobs()); }
            expired = weakRegistry.expired() && weakHeld.expired();
            queueEmpty = !hasPendingFinalizationJobs() && drainFinalizationJobs() == 0;
        } catch (...) { error = std::current_exception(); }
    }); worker.join(); if (error) std::rethrow_exception(error);
    REQUIRE(expired); REQUIRE(queueEmpty);
}
TEST_CASE("Weak lifetime operations reject foreign references before retaining heap storage", "[runtime][weaklife][threads]") {
    JobBoundary boundary; auto target = JSValue::object(), ref = createNativeWeakRef(target);
    auto registry = createNativeFinalizationRegistry(JSValue::function(noop));
    struct Foreign {
        JSValue target = JSValue::object(), held = JSValue::object(), callback = JSValue::function(noop);
        JSValue ref = createNativeWeakRef(target), registry = createNativeFinalizationRegistry(callback);
    };
    std::mutex mutex; std::condition_variable changed; Foreign* foreign = nullptr; bool done = false;
    std::thread worker([&] { Foreign values; std::unique_lock lock(mutex); foreign = &values; changed.notify_all(); changed.wait(lock, [&] { return done; }); });
    struct Join {
        std::thread& worker; std::mutex& mutex; std::condition_variable& changed; bool& done;
        ~Join() { { std::lock_guard lock(mutex); done = true; } changed.notify_all(); worker.join(); }
    } join{worker, mutex, changed, done};
    { std::unique_lock lock(mutex); changed.wait(lock, [&] { return foreign != nullptr; }); }
    REQUIRE_THROWS_AS(createNativeWeakRef(foreign->target), JSTypeError);
    REQUIRE_THROWS_AS(createNativeFinalizationRegistry(foreign->callback), JSTypeError);
    REQUIRE_THROWS_AS(JSWeakRef(foreign->ref), JSTypeError);
    REQUIRE_THROWS_AS(JSFinalizationRegistry(foreign->registry), JSTypeError);
    REQUIRE_THROWS_AS(JSFinalizationRegistry(registry).registerTarget(foreign->target, 1), JSTypeError);
    REQUIRE_THROWS_AS(JSFinalizationRegistry(registry).registerTarget(target, foreign->held), JSTypeError);
    REQUIRE_THROWS_AS(JSFinalizationRegistry(registry).registerTarget(target, 1, foreign->target), JSTypeError);
    REQUIRE_THROWS_AS(JSFinalizationRegistry(registry).unregister(foreign->target), JSTypeError);
    REQUIRE(JSWeakRef(ref).deref().sameValue(target));
}
TEST_CASE("Cleanup callbacks may collect and inspect pending weak tasks during dispatch", "[runtime][weaklife][gc]") {
    JobBoundary boundary; auto witness = JSValue::object(), log = JSValue::array();
    auto first = createNativeFinalizationRegistry(JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
        captures[0].collectGarbage();
        REQUIRE_FALSE(hasPendingFinalizationJobs());
        auto log = captures[1]; log.get("push").call(log, {"first"}); return {};
    }, {witness, log}));
    { auto target = JSValue::object(); JSFinalizationRegistry(first).registerTarget(target, 1); }
    witness.collectGarbage();
    auto second = createNativeFinalizationRegistry(recorder(log));
    { auto target = JSValue::object(); JSFinalizationRegistry(second).registerTarget(target, 2); }
    witness.collectGarbage(); second = {};
    REQUIRE(drainFinalizationJobs() == 1); REQUIRE(log.length() == 1);
    REQUIRE(log.get("0").strictEquals("first")); REQUIRE_FALSE(hasPendingFinalizationJobs());
}
