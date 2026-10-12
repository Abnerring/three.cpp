#include "catch.hpp"
#include "renderers/webgl/WebGLProperties.h"
#include "core/JSWeakMap.h"

namespace {
using Args = std::vector<JSValue>;
void append(JSValue events, JSValue value) { events.get("push").call(events, {value}); }
std::string trace(JSValue events) { return events.get("join").call(events, {","}).string(); }
}
TEST_CASE("WebGLProperties preserves identity and its old returned maps remain usable", "[renderers][webgl][properties]") {
    WebGLProperties properties; const auto key = JSValue::object(); REQUIRE_FALSE(properties.has(key).toBoolean());
    auto first = properties.get(key); REQUIRE(first.sameValue(properties.get(key))); REQUIRE(properties.has(key).toBoolean());
    REQUIRE(properties.update(key, "value", 12).isUndefined()); REQUIRE(first.get("value").number() == 12);
    REQUIRE(properties.remove(key).isUndefined()); REQUIRE_FALSE(properties.has(key).toBoolean());
    const auto next = properties.get(key); REQUIRE_FALSE(first.sameValue(next));
    REQUIRE(properties.dispose().isUndefined()); REQUIRE_FALSE(properties.has(key).toBoolean());
    first.set("old", 42); REQUIRE(first.get("value").number() == 12); REQUIRE(first.get("old").number() == 42);
    REQUIRE_FALSE(next.sameValue(properties.get(key)));
    const auto object = properties.object(); const auto get = object.get("get"), dispose = object.get("dispose");
    REQUIRE(get.call(nullptr, {key}).sameValue(properties.get(key))); dispose.call(false);
    REQUIRE_FALSE(properties.has(key).toBoolean());
}
TEST_CASE("WebGLProperties invalid keys follow WeakMap primitive rules", "[renderers][webgl][properties]") {
    WebGLProperties properties;
    for (const auto& key : Args{{}, nullptr, true, false, 0, -0.0, 1.5, "object"}) {
        REQUIRE_FALSE(properties.has(key).toBoolean()); REQUIRE(properties.remove(key).isUndefined());
        REQUIRE_THROWS_AS(properties.get(key), JSTypeError); REQUIRE_THROWS_AS(properties.update(key, "x", 1), JSTypeError);
    }
}
TEST_CASE("WebGLProperties update evaluates the cached base before property-key conversion", "[renderers][webgl][properties]") {
    WebGLProperties properties; auto object = JSValue::object(), events = JSValue::array(), key = JSValue::object();
    key.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args& args, const Args& captures)->JSValue {
        append(captures[0], args.at(0)); return "value";
    }, {events}));
    REQUIRE_THROWS_AS(properties.update(object, key, 42), JSTypeError); REQUIRE(trace(events).empty());
    auto map = properties.get(object); properties.update(object, key, 42);
    REQUIRE(trace(events) == "string"); REQUIRE(map.get("value").number() == 42);
    key.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args&, const Args&)->JSValue { throw JSRangeError("conversion"); }));
    REQUIRE_THROWS_AS(properties.update(object, key, 7), JSRangeError); REQUIRE(map.get("value").number() == 42);
}
TEST_CASE("WebGLProperties update retains its target when conversion disposes the live cache", "[renderers][webgl][properties]") {
    WebGLProperties properties; auto object = JSValue::object(), key = JSValue::object(); auto old = properties.get(object);
    key.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
        captures[0].get("dispose").call(); captures[0].collectGarbage(); return "after";
    }, {properties.object()}));
    REQUIRE(properties.update(object, key, 42).isUndefined()); REQUIRE(old.get("after").number() == 42);
    REQUIRE_FALSE(properties.has(object).toBoolean()); REQUIRE_FALSE(properties.get(object).sameValue(old));
}
TEST_CASE("WebGLProperties closures and literals keep upstream descriptors and dynamic WeakMap methods", "[renderers][webgl][properties]") {
    WebGLProperties properties; const auto object = properties.object();
    REQUIRE(object.keys() == std::vector<std::string>{"has", "get", "remove", "update", "dispose"});
    for (const auto* name : {"has", "get", "remove", "update", "dispose"}) {
        const auto descriptor = object.getOwnPropertyDescriptor(name);
        REQUIRE(descriptor->writable == true); REQUIRE(descriptor->enumerable == true); REQUIRE(descriptor->configurable == true);
    }
    auto prototype = nativeWeakMapConstructor().get("prototype");
    const auto previous = prototype.getOwnPropertyDescriptor("get");
    struct Restore { JSValue prototype; std::optional<JSPropertyDescriptor> previous;
        ~Restore() { if (previous) prototype.defineProperty("get", *previous); else prototype.erase("get"); }
    } restore{prototype, previous};
    JSPropertyDescriptor replacement; replacement.value = JSValue::function(+[](const JSValue& self, const Args&, const Args&)->JSValue {
        if (!isNativeWeakMap(self)) throw std::runtime_error("Lost managed WeakMap receiver"); return 17;
    }); replacement.writable = replacement.configurable = true;
    prototype.defineProperty("get", replacement);
    REQUIRE(properties.get(JSValue::object()).number() == 17);
    REQUIRE_THROWS_AS(properties.update(JSValue::object(), "x", 42), JSTypeError);
}
TEST_CASE("WebGLProperties ordinary closure construction preserves cache returns and disposal", "[renderers][webgl][properties][construct]") {
    WebGLProperties p;auto key=JSValue::object(),cached=p.get(key),custom=JSValue::object();cached.set("before",1);
    auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{throw std::runtime_error("newTarget body executed");},{},"Target",0,JSValue::ConstructorKind::Callable);
    target.set("prototype",custom);
    for(const auto& name:p.object().keys()) {
        const auto fn=p.object().get(name);REQUIRE(fn.isConstructible());
        REQUIRE(fn.ownPropertyNames()==std::vector<std::string>{"length","name","prototype"});
        REQUIRE(fn.get("prototype").get("constructor").sameValue(fn));
    }
    // An explicit object return supersedes the fresh receiver and its prototype.
    REQUIRE(p.object().get("get").construct({key},target).sameValue(cached));
    const auto hasResult=p.object().get("has").construct({key},target);
    REQUIRE(hasResult.getPrototype().sameValue(custom)); REQUIRE(hasResult.ownPropertyNames().empty());
    REQUIRE(p.object().get("update").construct({key,"x",42},target).getPrototype().sameValue(custom));
    REQUIRE(cached.get("x").number()==42);
    REQUIRE(p.object().get("dispose").construct({},target).getPrototype().sameValue(custom));
    REQUIRE_FALSE(p.has(key).toBoolean());REQUIRE(cached.get("before").number()==1);REQUIRE(cached.get("x").number()==42);
    REQUIRE_FALSE(p.get(key).sameValue(cached));
    REQUIRE_THROWS_AS(p.object().get("get").construct({17},target),JSTypeError);
}
TEST_CASE("WebGLProperties weak entries collect values which refer back to their keys", "[renderers][webgl][properties][gc]") {
    auto witness = JSValue::object(); WebGLProperties properties; JSWeakValue weakKey, weakMap;
    { auto key = JSValue::object(), map = properties.get(key); map.set("back", key); weakKey = JSWeakValue(key); weakMap = JSWeakValue(map); }
    witness.collectGarbage(); REQUIRE(weakKey.expired()); REQUIRE(weakMap.expired());
    auto key = JSValue::object(); JSWeakValue weakOld;
    { auto map = properties.get(key); weakOld = JSWeakValue(map); }
    witness.collectGarbage(); REQUIRE_FALSE(weakOld.expired()); properties.dispose(); witness.collectGarbage(); REQUIRE(weakOld.expired());
}
