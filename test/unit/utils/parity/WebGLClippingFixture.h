#pragma once
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "math/Matrix3.h"
#include "math/Plane.h"
#include "math/Vector3.h"
#include "renderers/webgl/WebGLClipping.h"
#include "renderers/webgl/WebGLProperties.h"
#include <cmath>
namespace ClippingFixture {
using Args = std::vector<JSValue>;
using A = JsonValue::Array;
using O = JsonValue::Object;
inline void prop(JSValue object, const std::string& name, JSValue value) {
    JSPropertyDescriptor d;
    d.value = std::move(value);
    d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(name, d);
}
inline JSValue array(Args values = {}) {
    auto result = JSValue::array();
    for (unsigned i = 0; i < values.size(); ++i)
        prop(result, std::to_string(i), values[i]);
    return result;
}
inline JSValue object(std::initializer_list<std::pair<const char*, JSValue>> values = {}) {
    auto result = JSValue::object();
    for (const auto& [key, value] : values)
        prop(result, key, value);
    return result;
}
inline void push(JSValue target, JSValue value) {
    target.get("push").call(target, {value});
}
inline JSValue invoke(JSValue target, const char* name, Args args = {}) {
    return target.get(name).call(target, args);
}
inline JsonValue encode(const JSValue& v) {
    if (v.isUndefined())
        return "undefined";
    if (v.isNull())
        return nullptr;
    if (v.type() == JSValue::Type::Boolean)
        return v.toBoolean();
    if (v.type() == JSValue::Type::String)
        return v.string();
    if (v.type() == JSValue::Type::Number) {
        const auto n = v.number();
        if (std::isnan(n))
            return "NaN";
        if (std::isinf(n))
            return n < 0 ? "-Infinity" : "Infinity";
        if (n == 0 && std::signbit(n))
            return "-0";
        return n;
    }
    if (v.isCallable())
        return O{{"function", v.get("name").toString()}};
    if (v.isHostObject("three.AnimationArray")) {
        A values;
        for (unsigned i = 0; i < v.get("length").toNumber(); ++i)
            values.push_back(encode(v.get(std::to_string(i))));
        return O{{"typed", v.get("constructor").get("name").toString()}, {"values", values}};
    }
    if (v.type() == JSValue::Type::Array) {
        A values;
        for (unsigned i = 0; i < v.length(); ++i)
            values.push_back(encode(v.get(std::to_string(i))));
        return values;
    }
    O out;
    for (const auto& key : v.keys()) {
        const auto d = v.getOwnPropertyDescriptor(key);
        out[key] = d && d->value ? encode(*d->value) : JsonValue("accessor");
    }
    return out;
}
template <class F> JsonValue outcome(F action) {
    try {
        return encode(action());
    } catch (const JSTypeError&) {
        return O{{"error", "TypeError"}};
    } catch (const JSRangeError&) {
        return O{{"error", "RangeError"}};
    } catch (const std::exception& e) {
        return O{{"error", "Error"}, {"message", e.what()}};
    }
}
inline JSValue matrix(unsigned mode) {
    auto e = array({1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
    switch (mode) {
    case 1:
        e.set("12", 2);
        e.set("13", -3);
        e.set("14", 4);
        break;
    case 2:
        e.set("0", 2);
        e.set("5", 3);
        e.set("10", 4);
        break;
    case 3:
        e.set("0", -2);
        e.set("5", .5);
        e.set("10", -1);
        break;
    case 4:
        e.set("0", 0);
        e.set("1", 1);
        e.set("4", -1);
        e.set("5", 0);
        break;
    case 5:
        e.set("4", .25);
        e.set("8", .125);
        e.set("9", -.5);
        e.set("12", 2);
        break;
    case 6:
        e.set("0", 0);
        e.set("5", 0);
        e.set("10", 0);
        break;
    case 7:
        e.set("3", .1);
        e.set("7", -.2);
        e.set("11", .3);
        e.set("15", 2);
        break;
    case 8:
        for (unsigned i = 0; i < 16; ++i)
            e.set(std::to_string(i),
                  (i % 5 == 0 ? 1. : 0.) + (static_cast<int>(i % 7) - 3) * .0625);
        break;
    case 9:
        e.set("0", 1e-20);
        e.set("5", 1e20);
        break;
    }
    return object({{"elements", e}});
}
inline JSValue plane(unsigned mode) {
    Args n{1, 0, 0};
    JSValue c = 2;
    switch (mode) {
    case 1:
        n = {1, 2, 3};
        c = -4;
        break;
    case 2:
        n = {0, -1, 0};
        c = 0;
        break;
    case 3:
        n = {0, 0, 0};
        c = 3;
        break;
    case 4:
        n = {-.1, .2, .3};
        c = -0.;
        break;
    case 5:
        n = {"2", false, nullptr};
        c = "-3";
        break;
    case 6:
        n = {1, JSValue(), 2};
        c = 1;
        break;
    case 7:
        n = {1, 2, 3};
        c = std::nan("");
        break;
    }
    return object({{"normal", object({{"x", n[0]}, {"y", n[1]}, {"z", n[2]}})}, {"constant", c}});
}
struct Fixture {
    JSValue properties = WebGLProperties().object(), manager = WebGLClipping(properties).object(),
            camera = object({{"matrixWorldInverse", matrix(0)}}),
            local = array({plane(1), plane(2)}), globals = array({plane(0)}),
            material = object(
                {{"clippingPlanes", local}, {"clipIntersection", false}, {"clipShadows", false}}),
            cache = invoke(properties, "get", {material}), uniform = manager.get("uniform"),
            log = array(), state;
    Fixture()
        : state(object({{"manager", manager},
                        {"camera", camera},
                        {"globals", globals},
                        {"material", material},
                        {"log", log},
                        {"acted", false}})) {}
    JSValue call(const char* name, Args args = {}) {
        return invoke(manager, name, std::move(args));
    }
    JsonValue snapshot() {
        return O{{"numPlanes", encode(manager.get("numPlanes"))},
                 {"numIntersection", encode(manager.get("numIntersection"))},
                 {"uniform", encode(uniform)},
                 {"cache", encode(cache.get("clippingState"))},
                 {"sameCache", uniform.get("value").strictEquals(cache.get("clippingState"))}};
    }
};
inline JsonValue sequenceCase(unsigned m, unsigned p, unsigned mode) {
    Fixture f;
    f.camera.set("matrixWorldInverse", matrix(m));
    f.globals = array(mode % 3 == 0   ? Args{}
                      : mode % 3 == 1 ? Args{plane(p)}
                                      : Args{plane(p), plane((p + 1) % 8)});
    f.material.set("clipIntersection", mode % 2 == 0 ? JSValue(false) : JSValue("yes"));
    f.material.set("clipShadows", mode % 4 > 1);
    f.local.set("0", plane((p + 2) % 8));
    A snapshots, results;
    const auto step = [&](const char* name, Args args) {
        results.push_back(outcome([&] { return f.call(name, args); }));
        snapshots.push_back(f.snapshot());
    };
    step("init", {f.globals, mode < 9 ? JSValue(true) : JSValue(false)});
    step("setGlobalState", {f.globals, f.camera});
    const auto global = f.uniform.get("value");
    step("setState", {f.material, f.camera, false});
    const auto first = f.cache.get("clippingState");
    f.local.set("0", plane((p + 4) % 8));
    f.camera.set("matrixWorldInverse", matrix((m + 1) % 10));
    step("setState", {f.material, f.camera,
                      mode % 3 == 0   ? JSValue(true)
                      : mode % 3 == 1 ? JSValue(false)
                                      : JSValue("true")});
    const bool reused = first.strictEquals(f.cache.get("clippingState"));
    step("beginShadows", {});
    step("setState", {f.material, f.camera, true});
    step("endShadows", {});
    f.material.set("clippingPlanes", nullptr);
    step("setState", {f.material, f.camera, true});
    return O{{"kind", "sequence"},
             {"m", m},
             {"p", p},
             {"mode", mode},
             {"results", results},
             {"snapshots", snapshots},
             {"reused", reused},
             {"resetIdentity", f.uniform.get("value").strictEquals(global)}};
}
inline void action(JSValue state, unsigned action) {
    if (state.get("acted").toBoolean())
        return;
    state.set("acted", true);
    if (action == 1)
        throw JSRangeError("getter");
    if (action == 2)
        invoke(state.get("manager"), "beginShadows");
    if (action == 3)
        invoke(state.get("manager"), "init", {array(), false});
    if (action == 4)
        invoke(state.get("manager"), "setGlobalState", {state.get("globals"), state.get("camera")});
}
inline void watch(Fixture& f, JSValue target, std::string name, unsigned actionMode = 0) {
    const auto value = target.get(name);
    JSPropertyDescriptor d;
    d.enumerable = d.configurable = true;
    d.getter = JSValue::function(
        +[](const JSValue& self, const Args&, const Args& c) -> JSValue {
            push(c[0].get("log"), array({"get", c[2], self.strictEquals(c[1])}));
            action(c[0], static_cast<unsigned>(c[4].toNumber()));
            return c[3];
        },
        {f.state, target, name, value, actionMode});
    d.setter = JSValue::function(
        +[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            push(c[0].get("log"), array({"set", c[2], self.strictEquals(c[1])}));
            prop(self, c[2].toString(), args[0]);
            return {};
        },
        {f.state, target, name});
    target.defineProperty(name, d);
}
inline JsonValue observedCase(unsigned point, unsigned actionMode) {
    Fixture f;
    f.local = object({{"0", f.local.get("0")}, {"1", f.local.get("1")}, {"length", 2}});
    f.material.set("clippingPlanes", f.local);
    f.call("init", {f.globals, true});
    f.call("setGlobalState", {f.globals, f.camera});
    if (point < 3)
        watch(f, f.material,
              std::vector<std::string>{"clippingPlanes", "clipIntersection", "clipShadows"}[point],
              actionMode);
    else if (point == 3)
        watch(f, f.properties, "get", actionMode);
    else if (point == 4)
        watch(f, f.local, "length", actionMode);
    else if (point == 5)
        watch(f, f.camera, "matrixWorldInverse", actionMode);
    else if (point == 6)
        watch(f, f.camera.get("matrixWorldInverse"), "elements", actionMode);
    else if (point >= 7 && point < 23)
        watch(f, f.camera.get("matrixWorldInverse").get("elements"), std::to_string(point - 7),
              actionMode);
    else if (point == 23)
        watch(f, f.local, "0", actionMode);
    else if (point == 24)
        watch(f, f.local.get("0"), "normal", actionMode);
    else if (point == 25)
        watch(f, f.local.get("0"), "constant", actionMode);
    else if (point >= 26 && point < 29)
        watch(f, f.local.get("0").get("normal"),
              std::vector<std::string>{"x", "y", "z"}[point - 26], actionMode);
    else if (point == 29)
        watch(f, f.cache, "clippingState", actionMode);
    else if (point == 30)
        watch(f, f.uniform, "value", actionMode);
    else if (point == 31)
        watch(f, f.uniform, "needsUpdate", actionMode);
    else if (point == 32)
        watch(f, f.manager, "numPlanes", actionMode);
    else if (point == 33)
        watch(f, f.manager, "numIntersection", actionMode);
    const auto result = outcome([&] { return f.call("setState", {f.material, f.camera, false}); });
    const auto trace = encode(f.log);
    return O{{"kind", "observed"}, {"point", point}, {"action", actionMode},
             {"result", result},   {"trace", trace}, {"snapshot", f.snapshot()}};
}
inline JsonValue metadata(JSValue function) {
    A own;
    for (const auto& key : function.ownPropertyNames())
        own.emplace_back(key);
    O out{{"name", encode(function.get("name"))},
          {"length", encode(function.get("length"))},
          {"own", own},
          {"constructible", function.isConstructible()}};
    if (const auto d = function.getOwnPropertyDescriptor("prototype"))
        out["prototypeFlags"] = A{*d->writable, *d->enumerable, *d->configurable};
    return out;
}
inline JsonValue keys(JSValue value) {
    A result;
    for (const auto& key : value.keys())
        result.emplace_back(key);
    return result;
}
inline const std::vector<std::string>& methods() {
    static const std::vector<std::string> x{"init", "beginShadows", "endShadows", "setGlobalState",
                                            "setState"};
    return x;
}
inline JsonValue constructCase(unsigned method, unsigned mode) {
    Fixture f;
    f.call("init", {f.globals, true});
    f.call("setGlobalState", {f.globals, f.camera});
    auto function =
        method == 5 ? nativeWebGLClippingConstructor() : f.manager.get(methods()[method]);
    Args args;
    if (method == 0)
        args = {f.globals, true};
    if (method == 3)
        args = {f.globals, f.camera};
    if (method == 4)
        args = {f.material, f.camera, false};
    if (method == 5)
        args = {f.properties};
    auto other = JSValue::constructor(
             +[](const JSValue&, const Args&, const Args&) -> JSValue {
                 throw JSRangeError("newTarget body");
             },
             {}, "Other", 0, JSValue::ConstructorKind::Callable),
         prototype = object({{"numPlanes", 10}});
    other.set("prototype", mode == 2 ? JSValue(nullptr) : prototype);
    JSValue result;
    const auto status = outcome([&]() -> JSValue {
        if (mode == 0)
            result = function.construct(args);
        else if (mode < 3)
            result = function.construct(args, other);
        else if (mode == 3)
            result = function.call(prototype, args);
        else
            result = function.call(JSValue(), args);
        return {};
    });
    return O{
        {"kind", "construct"},
        {"method", method},
        {"mode", mode},
        {"metadata", metadata(function)},
        {"status", status},
        {"result", result.isObject()
                       ? JsonValue(O{{"keys", keys(result)},
                                     {"numPlanes", encode(result.get("numPlanes"))},
                                     {"numIntersection", encode(result.get("numIntersection"))}})
                       : encode(result)},
        {"customPrototype", result.isObject() && result.getPrototype().strictEquals(prototype)},
        {"receiver", O{{"numPlanes", encode(prototype.get("numPlanes"))},
                       {"numIntersection", encode(prototype.get("numIntersection"))}}},
        {"snapshot", f.snapshot()}};
}

inline JSValue scalar(unsigned n) {
    switch (n) {
    case 0:
        return JSValue();
    case 1:
        return nullptr;
    case 2:
        return false;
    case 3:
        return true;
    case 4:
        return 0;
    case 5:
        return 1;
    case 6:
        return -0.;
    case 7:
        return "";
    case 8:
        return "0";
    case 9:
        return std::nan("");
    case 10:
        return object();
    case 11:
        return array();
    }
    return {};
}
inline JsonValue initCase(unsigned length, unsigned enabled) {
    Fixture f;
    auto planes = object({{"length", scalar(length)}});
    A results;
    results.push_back(outcome([&] { return f.call("init", {planes, scalar(enabled)}); }));
    results.push_back(outcome([&] { return f.call("init", {array(), false}); }));
    results.push_back(outcome([&] { return f.call("init", {array(), false}); }));
    return O{{"kind", "init"},
             {"length", length},
             {"enabled", enabled},
             {"results", results},
             {"snapshot", f.snapshot()}};
}
inline JsonValue cacheCase(unsigned mode, unsigned skip) {
    Fixture f;
    f.call("init", {f.globals, true});
    f.call("setGlobalState", {f.globals, f.camera});
    JSValue value = scalar(mode);
    if (mode == 12)
        value = object({{"length", 0}});
    if (mode == 13)
        value = array({7});
    if (mode == 14)
        value = NativeArray_detail::constructor(6).construct({1});
    if (mode == 15)
        value = NativeArray_detail::constructor(6).construct({20});
    if (mode == 16)
        value = object({{"length", 20}});
    if (mode == 17)
        value = array(Args(20, 7));
    if (mode == 18)
        value = NativeArray_detail::constructor(4).construct({20});
    if (mode >= 19)
        value = object({{"length", 20}});
    if (mode == 19) {
        JSPropertyDescriptor d;
        d.value = 7;
        d.writable = false;
        value.defineProperty("4", d);
    }
    if (mode == 20)
        f.manager.set("uniform",
                      object({{"value", "replacement"}, {"needsUpdate", "replacement"}}));
    if (mode == 21) {
        JSPropertyDescriptor d;
        d.writable = false;
        f.manager.defineProperty("numPlanes", d);
    }
    if (mode == 22) {
        JSPropertyDescriptor d;
        d.setter = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue {
            throw JSRangeError("cache setter");
        });
        d.configurable = true;
        f.cache.defineProperty("clippingState", d);
    } else
        prop(f.cache, "clippingState", value);
    if (mode == 23)
        f.cache.preventExtensions();
    const auto result = outcome([&] {
        return f.call("setState", {f.material, f.camera,
                                   skip == 0   ? JSValue(false)
                                   : skip == 1 ? JSValue(true)
                                               : JSValue("true")});
    });
    const auto first = f.snapshot();
    const auto again = outcome([&] { return f.call("setState", {f.material, f.camera, true}); });
    return O{{"kind", "cache"},
             {"mode", mode},
             {"skip", skip},
             {"result", result},
             {"first", first},
             {"again", again},
             {"snapshot", f.snapshot()},
             {"same", f.cache.get("clippingState").strictEquals(value)},
             {"publicUniform", encode(f.manager.get("uniform"))}};
}
inline const std::vector<std::string>& mathMethods() {
    static const std::vector<std::string> x{
        "getNormalMatrix", "setFromMatrix4", "invert", "transpose",    "copy",
        "applyMatrix4",    "coplanarPoint",  "copy",   "applyMatrix4", "applyMatrix3",
        "normalize",       "toArray",        "dot"};
    return x;
}
inline JSValue mathPrototype(unsigned point) {
    if (point < 4)
        return nativeMatrix(std::make_shared<Matrix3>()).getPrototype();
    if (point < 7)
        return nativePlane(std::make_shared<Plane>()).getPrototype();
    return nativeVector(std::make_shared<Vector3>()).getPrototype();
}
inline JsonValue methodCase(unsigned point, unsigned actionMode) {
    Fixture f;
    f.call("init", {f.globals, true});
    f.call("setGlobalState", {f.globals, f.camera});
    auto prototype = mathPrototype(point);
    const auto name = mathMethods()[point];
    const auto descriptor = prototype.getOwnPropertyDescriptor(name);
    const auto original = prototype.get(name);
    auto wrapper = JSValue::function(
        +[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            push(c[0].get("log"), array({"call", c[2]}));
            action(c[0], static_cast<unsigned>(c[3].toNumber()));
            return c[1].call(self, args);
        },
        {f.state, original, name, actionMode}, name, original.get("length").toNumber());
    prop(prototype, name, wrapper);
    const auto result = outcome([&] { return f.call("setState", {f.material, f.camera, false}); });
    prototype.defineProperty(name, *descriptor);
    return O{{"kind", "method"}, {"point", point},         {"action", actionMode},
             {"result", result}, {"trace", encode(f.log)}, {"snapshot", f.snapshot()}};
}

inline JsonValue assignmentCase(unsigned point, unsigned mode) {
    Fixture f;
    const std::vector<std::string> fields{"uniform",
                                          "numPlanes",
                                          "numIntersection",
                                          "init",
                                          "beginShadows",
                                          "endShadows",
                                          "setGlobalState",
                                          "setState",
                                          "isPlane",
                                          "normal",
                                          "constant",
                                          "x",
                                          "y",
                                          "z",
                                          "elements"};
    const auto field = fields[point];
    auto prototype = point < 8    ? object()
                     : point < 11 ? mathPrototype(4)
                     : point < 14 ? mathPrototype(7)
                                  : mathPrototype(0);
    auto other = JSValue::constructor(
        +[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }, {}, "Other", 0,
        JSValue::ConstructorKind::Callable);
    other.set("prototype", prototype);
    const auto old = prototype.getOwnPropertyDescriptor(field);
    JSPropertyDescriptor d;
    d.configurable = true;
    if (mode == 2) {
        d.value = "readonly";
        d.writable = false;
    } else {
        d.setter = JSValue::function(
            +[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                push(c[0], array({c[1], args[0].isObject() ? JSValue("object") : args[0]}));
                if (c[2].strictEquals(1))
                    throw JSRangeError("constructor setter");
                prop(self, c[1].toString(), args[0]);
                return {};
            },
            {f.log, field, mode});
    }
    prototype.defineProperty(field, d);
    JSValue built;
    const auto status = outcome([&]() -> JSValue {
        built = point < 8 ? nativeWebGLClippingConstructor().construct({f.properties}, other)
                          : nativeWebGLClippingConstructor().construct({f.properties});
        return {};
    });
    if (old)
        prototype.defineProperty(field, *old);
    else
        prototype.erase(field);
    return O{{"kind", "assignment"},
             {"point", point},
             {"mode", mode},
             {"status", status},
             {"trace", encode(f.log)},
             {"builtKeys", built.isObject() ? keys(built) : JsonValue(nullptr)}};
}
} // namespace ClippingFixture
