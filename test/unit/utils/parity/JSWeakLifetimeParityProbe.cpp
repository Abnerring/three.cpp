#include "core/JSWeakRef.h"
#include "core/JSFinalizationRegistry.h"
#include "core/JSJobs.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args = std::vector<JSValue>; using O = JsonValue::Object; using A = JsonValue::Array;
JSValue noop(const JSValue&, const Args&, const Args&) { return {}; }
Args values() { return {{}, nullptr, false, true, 0, -0.0, 1.5, std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), "", "value",
    JSValue::object(), JSValue::array(), JSValue::function(noop)}; }
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "undefined"; if (value.isNull()) return nullptr;
    if (value.type() == JSValue::Type::String) return value.string();
    if (value.type() == JSValue::Type::Boolean) return value.toBoolean();
    if (value.type() == JSValue::Type::Number) {
        if (std::isnan(value.number())) return "NaN";
        if (std::isinf(value.number())) return value.number() < 0 ? "-Infinity" : "Infinity";
        if (value.number() == 0 && std::signbit(value.number())) return "-0";
        return value.number();
    }
    return value.isCallable() ? "function" : value.type() == JSValue::Type::Array ? "array" : "object";
}
template<class F> JsonValue invoke(F&& callback) {
    try { return encode(callback()); } catch (const JSTypeError&) { return O{{"error", "TypeError"}}; }
}
JSValue call(const JSValue& object, const char* name, const Args& args = {}) { return object.get(name).call(object, args); }
JSValue recorder(const JSValue& log) {
    return JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures)->JSValue {
        auto log = captures[0]; log.get("push").call(log, {self.isUndefined(), args[0]}); return {};
    }, {log});
}
void emit(JsonValue value) { std::cout << value.dump() << '\n'; }
void gc(const JSValue& witness) { endJSJob(); witness.collectGarbage(); }
}
int main() {
    const Args constructors{nativeWeakRefConstructor(), nativeFinalizationRegistryConstructor()};
    for (unsigned c = 0; c < 2; ++c) {
        const auto ctor = constructors[c], prototype = ctor.get("prototype");
        auto descriptor = ctor.getOwnPropertyDescriptor("prototype");
        emit(O{{"kind", "constructor"}, {"index", c}, {"name", encode(ctor.get("name"))}, {"length", encode(ctor.get("length"))},
            {"call", invoke([&] { return ctor.call(); })}, {"constructor", prototype.get("constructor").sameValue(ctor)},
            {"writable", *descriptor->writable}, {"enumerable", *descriptor->enumerable}, {"configurable", *descriptor->configurable}});
        for (unsigned i = 0; i < 15; ++i) {
            auto value = values()[i];
            emit(O{{"kind", "construct"}, {"constructor", c}, {"index", i}, {"result", invoke([&] { return ctor.construct({value}); })}});
        }
        auto newTarget = JSValue::constructor(noop, {}, "Target", 0, JSValue::ConstructorKind::Callable);
        newTarget.set("prototype", 3);
        const auto argument = c == 0 ? JSValue::object() : JSValue::function(noop);
        const auto fallback = ctor.construct({argument}, newTarget), custom = JSValue::object();
        newTarget.set("prototype", custom); const auto derived = ctor.construct({argument}, newTarget);
        emit(O{{"kind", "newTarget"}, {"constructor", c}, {"fallback", fallback.getPrototype().sameValue(prototype)},
            {"custom", derived.getPrototype().sameValue(custom)}, {"brand", c == 0 ? isNativeWeakRef(derived) : isNativeFinalizationRegistry(derived)}});
    }
    for (const auto& [ctor, name] : std::vector<std::pair<JSValue, const char*>>{
            {constructors[0], "deref"}, {constructors[1], "register"}, {constructors[1], "unregister"}}) {
        const auto prototype = ctor.get("prototype"), method = prototype.get(name);
        const auto descriptor = prototype.getOwnPropertyDescriptor(name);
        emit(O{{"kind", "method"}, {"method", name}, {"name", encode(method.get("name"))}, {"length", encode(method.get("length"))},
            {"construct", invoke([&] { return method.construct(); })}, {"prototype", method.hasOwn("prototype")},
            {"writable", *descriptor->writable}, {"enumerable", *descriptor->enumerable}, {"configurable", *descriptor->configurable}});
        for (unsigned i = 0; i < 15; ++i) {
            const auto receiver = values()[i];
            emit(O{{"kind", "brand"}, {"method", name}, {"index", i}, {"result", invoke([&] { return method.call(receiver, {nullptr}); })}});
        }
    }
    for (unsigned kind = 12; kind < 15; ++kind) {
        const auto target = values()[kind], ref = createNativeWeakRef(target);
        emit(O{{"kind", "identity"}, {"index", kind}, {"same", call(ref, "deref").sameValue(target)},
            {"ownKeys", ref.keys().size()}, {"prototypeBrand", isNativeWeakRef(constructors[0].get("prototype"))}});
    }
    const auto registry = createNativeFinalizationRegistry(JSValue::function(noop));
    const auto all = values();
    for (unsigned target = 0; target < 15; ++target) for (unsigned held = 0; held < 15; ++held) for (unsigned token = 0; token < 15; ++token)
        emit(O{{"kind", "register"}, {"target", target}, {"held", held}, {"token", token},
            {"result", invoke([&] { return call(registry, "register", {all[target], all[held], all[token]}); })}});
    for (unsigned token = 0; token < 15; ++token)
        emit(O{{"kind", "unregister"}, {"token", token}, {"first", invoke([&] { return call(registry, "unregister", {all[token]}); })},
            {"again", invoke([&] { return call(registry, "unregister", {all[token]}); })}});
    {
        auto log = JSValue::array(), target = JSValue::object();
        target.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
            auto log = captures[0]; log.get("push").call(log, {1}); throw JSTypeError("coercion");
        }, {log}));
        const auto ref = createNativeWeakRef(target);
        call(registry, "register", {target, "held", target});
        emit(O{{"kind", "noCoercion"}, {"same", call(ref, "deref").sameValue(target)},
            {"removed", encode(call(registry, "unregister", {target}))}, {"conversions", log.length()}});
    }
    endJSJob();
    auto witness = JSValue::object();
    {
        auto target = JSValue::object(), ref = createNativeWeakRef(target); target = {};
        witness.collectGarbage(); const auto sameJob = call(ref, "deref").isObject();
        auto log = JSValue::array();
        queueMicrotask(JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
            captures[1].collectGarbage(); auto log = captures[2];
            log.get("push").call(log, {JSWeakRef(captures[0]).deref().isObject()}); return {};
        }, {ref, witness, log}));
        drainMicrotasks(); gc(witness);
        emit(O{{"kind", "gcKeepalive"}, {"sameJob", sameJob}, {"microtask", log.get("0").toBoolean()}, {"nextJobDead", call(ref, "deref").isUndefined()}});
    }
    {
        auto log = JSValue::array(), target = JSValue::object(), held = JSValue::object();
        held.set("marker", 42); auto state = JSValue::object(); state.set("held", held);
        auto callback = JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures)->JSValue {
            auto log = captures[0]; log.get("push").call(log, {self.isUndefined(), args[0].sameValue(captures[1].get("held")), args[0].get("marker")}); return {};
        }, {log, state});
        const auto reg = createNativeFinalizationRegistry(callback), ref = createNativeWeakRef(target);
        call(reg, "register", {target, held}); target = {}; const auto sync = log.length() == 0;
        gc(witness); const auto queued = log.length() == 0; drainFinalizationJobs();
        emit(O{{"kind", "gcCleanup"}, {"syncEmpty", sync}, {"queuedEmpty", queued}, {"dead", call(ref, "deref").isUndefined()},
            {"receiver", log.get("0").toBoolean()}, {"identity", log.get("1").toBoolean()}, {"marker", encode(log.get("2"))}, {"count", log.length() / 3}});
    }
    {
        auto log = JSValue::array(), target = JSValue::object(), token = JSValue::object();
        const auto reg = createNativeFinalizationRegistry(recorder(log)), ref = createNativeWeakRef(target);
        call(reg, "register", {target, "a", token}); call(reg, "register", {target, "b", token});
        const auto removed = call(reg, "unregister", {token}).toBoolean(); target = {}; gc(witness); drainFinalizationJobs();
        emit(O{{"kind", "gcUnregister"}, {"removed", removed}, {"dead", call(ref, "deref").isUndefined()}, {"calls", log.length()}});
    }
    {
        auto log = JSValue::array(), target = JSValue::object(), token = JSValue::object();
        const auto reg = createNativeFinalizationRegistry(recorder(log)), ref = createNativeWeakRef(target);
        call(reg, "register", {target, "held", token}); target = {}; gc(witness);
        const auto removed = call(reg, "unregister", {token}).toBoolean(); drainFinalizationJobs();
        emit(O{{"kind", "gcPendingUnregister"}, {"removed", removed}, {"dead", call(ref, "deref").isUndefined()}, {"calls", log.length()}});
    }
    {
        auto log = JSValue::array(), target = JSValue::object(), held = JSValue::object(), token = JSValue::object();
        const auto reg = createNativeFinalizationRegistry(recorder(log)), ref = createNativeWeakRef(target);
        held.set("target", target); call(reg, "register", {target, held, token}); target = {}; held = {}; gc(witness);
        const auto alive = call(ref, "deref").isObject(); call(reg, "unregister", {token}); gc(witness);
        emit(O{{"kind", "gcHeldReachability"}, {"alive", alive}, {"afterUnregisterDead", call(ref, "deref").isUndefined()}, {"calls", log.length()}});
    }
    {
        auto log = JSValue::array(); JSValue refRegistry, refHeld;
        { auto target = JSValue::object(), held = JSValue::object(), reg = createNativeFinalizationRegistry(recorder(log));
          held.set("registry", reg); call(reg, "register", {target, held});
          refRegistry = createNativeWeakRef(reg); refHeld = createNativeWeakRef(held); target = {}; gc(witness); }
        gc(witness); drainFinalizationJobs();
        emit(O{{"kind", "gcRegistryCycle"}, {"registryDead", call(refRegistry, "deref").isUndefined()},
            {"heldDead", call(refHeld, "deref").isUndefined()}, {"calls", log.length()}});
    }
    {
        auto target = JSValue::object(), token = JSValue::object(), log = JSValue::array();
        const auto reg = createNativeFinalizationRegistry(recorder(log)), refToken = createNativeWeakRef(token);
        call(reg, "register", {target, "held", token}); token = {}; gc(witness);
        const auto tokenDead = call(refToken, "deref").isUndefined(); target = {}; gc(witness); drainFinalizationJobs();
        emit(O{{"kind", "gcWeakToken"}, {"tokenDead", tokenDead}, {"calls", log.length() / 2}, {"held", encode(log.get("1"))}});
    }
    endJSJob(); witness.collectGarbage(); drainFinalizationJobs();
}
