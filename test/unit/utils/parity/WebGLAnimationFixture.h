#pragma once
#include "renderers/webgl/WebGLAnimation.h"
#include <string>

namespace WebGLAnimationParity {
using Args = std::vector<JSValue>;
using A = JsonValue::Array;
using O = JsonValue::Object;
inline JSValue call(const JSValue& object, const char* method, const Args& args = {}) {
    return object.get(method).call(object, args);
}
inline JSValue array(const Args& values) {
    auto result = JSValue::array();
    for (const auto& value : values) result.set(std::to_string(result.length()), value);
    return result;
}
inline JSValue value(const JSValue& v) { return v.isUndefined() ? JSValue("undefined") : v; }
inline void event(JSValue state, const Args& values) {
    auto trace = state.get("trace");
    trace.set(std::to_string(trace.length()), array(values));
    const auto n = state.get("events").number() + 1;
    state.set("events", n);
    if (n == state.get("failAt").number()) throw JSRangeError("event");
}
inline JSValue loop(const JSValue& self, const Args& args, const Args& captures) {
    auto state = captures[0];
    event(state, {"loop", self.isUndefined(), value(args[0]), args[1].sameValue(state.get("frame"))});
    const auto mode = state.get("mode").number();
    if (mode == 10) call(state.get("animation"), "stop");
    if (mode == 11) {
        call(state.get("animation"), "setContext", {state.get("second")});
        call(state.get("animation"), "setAnimationLoop", {JSValue::function(
            +[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                event(captures[0], {"replacement-loop", self.isUndefined(), value(args[0])}); return {};
            }, {state})});
    }
    if (mode == 14) throw JSRangeError("loop");
    return {};
}
inline JSValue context(JSValue state, int id) {
    auto result = JSValue::object();
    result.set("id", id);
    for (bool cancel : {false, true}) {
        JSPropertyDescriptor descriptor;
        descriptor.configurable = descriptor.enumerable = true;
        descriptor.getter = JSValue::function(
            +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
                auto state = captures[0]; const auto id = captures[1]; const bool cancel = captures[2].toBoolean();
                event(state, {cancel ? "get-cancel" : "get-request", id});
                const auto mode = state.get("mode").number();
                if (!cancel && !state.get("acted").toBoolean()) {
                    state.set("acted", true);
                    if (mode == 6) call(state.get("animation"), "setContext", {state.get("second")});
                    if (mode == 16) throw JSRangeError("request-getter");
                }
                if (cancel && mode == 12) call(state.get("animation"), "start");
                return JSValue::function(
                    +[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                        auto state = captures[0]; const auto id = captures[1]; const bool cancel = captures[2].toBoolean();
                        const auto mode = state.get("mode").number();
                        if (cancel) {
                            event(state, {"cancel", id, self.get("id"), value(args[0])});
                            if (mode == 13) throw JSRangeError("cancel");
                            return {};
                        }
                        event(state, {"request", id, self.get("id"), args[0].get("name"), args[0].get("length")});
                        const auto n = state.get("requests").number() + 1;
                        state.set("requests", n); state.set("pending", args[0]);
                        if (n == 1) {
                            if (mode == 7) call(state.get("animation"), "stop");
                            if (mode == 8) call(state.get("animation"), "setAnimationLoop", {nullptr});
                            if (mode == 9) call(state.get("animation"), "setContext", {state.get("second")});
                            if (mode == 15) throw JSRangeError("request");
                        }
                        return n + 100;
                    }, {state, id, cancel});
            }, {state, id, cancel});
        result.defineProperty(cancel ? "cancelAnimationFrame" : "requestAnimationFrame", descriptor);
    }
    return result;
}
template<class F> inline std::string invoke(F&& function) {
    try { const auto result = function(); return result.isUndefined() ? "undefined" : "value"; }
    catch (const JSRangeError&) { return "RangeError"; }
    catch (const JSTypeError&) { return "TypeError"; }
}
inline JsonValue scenario(int mode, int failAt) {
    auto state = JSValue::object(nullptr);
    state.set("trace", JSValue::array()); state.set("events", 0); state.set("requests", 0);
    state.set("mode", mode); state.set("failAt", failAt); state.set("acted", false);
    state.set("frame", JSValue::object());
    auto animation = nativeWebGLAnimationConstructor().call({}); state.set("animation", animation);
    state.set("second", context(state, 2)); auto first = context(state, 1);
    A results;
    const auto run = [&](const char* name, const Args& args = {}) {
        results.emplace_back(A{name, invoke([&] { return call(animation, name, args); })});
    };
    run("start"); run("stop");
    if (mode == 1) run("setAnimationLoop", {JSValue::function(loop, {state})});
    run("setContext", {mode == 4 ? JSValue() : mode == 5 ? JSValue(nullptr) : mode == 18 ? JSValue::object() : first});
    run("start");
    run("setAnimationLoop", {mode == 2 ? JSValue(nullptr) : mode == 3 ? JSValue() : mode == 17 ? JSValue(23) : JSValue::function(loop, {state})});
    run("start"); run("start");
    for (int tick = 0; tick < 2; ++tick) {
        const auto pending = state.get("pending");
        results.emplace_back(A{"tick", pending.isUndefined() ? "absent" : invoke([&] {
            return pending.call({}, {tick == 0 ? JSValue(12) : JSValue(), state.get("frame")});
        })});
    }
    run("stop"); run("start"); run("stop"); run("setContext", {nullptr}); run("stop");
    run("setAnimationLoop", {nullptr}); run("start");
    auto result = O{{"mode", mode}, {"failAt", failAt}, {"results", results},
        {"trace", *state.get("trace").toJSON()}, {"events", state.get("events").number()}};
    animation = {}; first = {}; state = {};
    JSValue::object().collectGarbage();
    return result;
}
inline JsonValue flags(const JSValue& object, const char* name) {
    const auto descriptor = object.getOwnPropertyDescriptor(name);
    return A{descriptor->writable.value_or(false), descriptor->enumerable.value_or(false), descriptor->configurable.value_or(false)};
}
inline JsonValue metadata() {
    const auto constructor = nativeWebGLAnimationConstructor();
    const auto animation = constructor.construct();
    A methods;
    for (const auto* name : {"start", "stop", "setAnimationLoop", "setContext"}) {
        const auto method = animation.get(name);
        methods.emplace_back(O{{"name", method.get("name").toString()}, {"length", method.get("length").number()},
            {"flags", flags(animation, name)}, {"prototypeFlags", flags(method, "prototype")},
            {"constructor", method.get("prototype").get("constructor").sameValue(method)}});
    }
    return O{{"kind", "metadata"}, {"name", constructor.get("name").toString()},
        {"length", constructor.get("length").number()}, {"methods", methods}};
}
}
