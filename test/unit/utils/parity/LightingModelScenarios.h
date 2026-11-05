#pragma once

#include "nodes/core/LightingModel.h"
#include "nodes/functions/BasicLightingModel.h"
#include "nodes/functions/PhongLightingModel.h"

#include <string>
#include <vector>

namespace LightingModelScenarios {
using Args = std::vector<JSValue>;
inline JSValue object() { return JSValue::object(nullptr); }
inline void push(JSValue array, const JSValue& value) { array.set(array.length(), value); }
inline JSValue call(const JSValue& self, const char* key, const Args& args = {}) { return self.get(key).call(self, args); }
inline JSValue names(const std::vector<std::string>& source) {
    auto out = JSValue::array(); for (const auto& key : source) push(out, key); return out;
}
inline const std::vector<std::string>& modes() {
    static const std::vector<std::string> values{
        "baseline", "throw:get lightsNode:1", "throw:get setupLights", "throw:get lightsNode:2",
        "throw:get getLightNodes", "throw:call getLightNodes", "throw:call setupLights",
        "throw:get indirect", "throw:call indirect", "badSetup", "badGetNodes", "badIndirect",
        "nullFirst", "nullSecond", "reenter:call getLightNodes", "reenter:call setupLights",
        "reenter:call indirect", "replaceSetup"};
    return values;
}
inline void tick(const JSValue& state, const std::string& label) {
    push(state.get("events"), label);
    if (state.get("mode").strictEquals("throw:" + label)) throw JSTypeError(label);
}
inline void reenter(JSValue state, const std::string& label) {
    if (state.get("mode").strictEquals("reenter:" + label) && !state.get("nested").toBoolean()) {
        state.set("nested", true);
        state.get("start").call(state.get("self"), {state.get("builder")});
    }
}
inline JSValue descriptor(const JSPropertyDescriptor& d) {
    auto out = object();
    out.set("writable", d.writable ? JSValue(*d.writable) : JSValue("@absent"));
    out.set("enumerable", d.enumerable.value_or(false));
    out.set("configurable", d.configurable.value_or(false));
    out.set("getter", d.getter && !d.getter->isUndefined());
    out.set("setter", d.setter && !d.setter->isUndefined());
    return out;
}
inline JSValue metadata() {
    const auto ctor = nativeLightingModelConstructor(), prototype = ctor.get("prototype");
    auto out = object(), methods = JSValue::array(), statics = JSValue::array();
    out.set("exports", names(nativeLightingModelModule().keys()));
    out.set("name", ctor.get("name")); out.set("length", ctor.get("length"));
    out.set("constructible", ctor.isConstructible()); out.set("source", call(ctor, "toString"));
    out.set("staticNames", names(ctor.ownPropertyNames()));
    out.set("prototypeNames", names(prototype.ownPropertyNames()));
    for (const auto& key : ctor.ownPropertyNames()) {
        auto row = descriptor(*ctor.getOwnPropertyDescriptor(key)); row.set("key", key);
        if (key != "prototype") row.set("value", ctor.get(key));
        push(statics, row);
    }
    for (const auto& key : prototype.ownPropertyNames()) {
        const auto fn = prototype.get(key);
        auto row = descriptor(*prototype.getOwnPropertyDescriptor(key)); row.set("key", key);
        row.set("name", fn.get("name")); row.set("length", fn.get("length"));
        row.set("constructible", fn.isConstructible()); row.set("source", call(fn, "toString"));
        push(methods, row);
    }
    out.set("statics", statics); out.set("methods", methods);
    return out;
}
inline JSValue constructionAndHooks() {
    const auto ctor = nativeLightingModelConstructor(), prototype = ctor.get("prototype");
    const auto first = ctor.construct({17}), second = ctor.construct();
    auto out = object();
    out.set("instanceOwnNames", names(first.ownPropertyNames()));
    out.set("distinct", !first.sameValue(second)); out.set("notConstructor", !first.sameValue(ctor));
    out.set("notCallable", !first.isCallable()); out.set("prototype", first.getPrototype().sameValue(prototype));
    out.set("constructor", first.get("constructor").sameValue(ctor));
    out.set("instanceof", ctor.ordinaryHasInstance(first));
    out.set("objectPrototype", prototype.getPrototype().sameValue(JSValue::object().getPrototype()));
    const auto dummy = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
    out.set("functionPrototype", ctor.getPrototype().sameValue(dummy.getPrototype()));
    auto derived = JSValue::constructor(
        +[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }, {}, "Derived", 0,
        JSValue::ConstructorKind::Callable);
    const auto derivedInstance = ctor.construct({}, derived);
    out.set("newTargetPrototype", derivedInstance.getPrototype().sameValue(derived.get("prototype")));
    out.set("newTargetConstructor", derivedInstance.get("constructor").sameValue(derived));
    derived.set("prototype", 7);
    out.set("primitivePrototypeFallback", ctor.construct({}, derived).getPrototype().sameValue(JSValue::object().getPrototype()));
    try { ctor.call({}, {}); out.set("bareCall", "value"); }
    catch (const JSTypeError&) { out.set("bareCall", "TypeError"); }
    auto empty = JSValue::array();
    for (const auto* key : {"finish", "direct", "directRectArea", "indirect", "ambientOcclusion"}) {
        const auto fn = prototype.get(key);
        auto row = object(); row.set("key", key);
        row.set("undefined", fn.call(nullptr, {nullptr, 17, false}).isUndefined());
        try { fn.construct(); row.set("new", "value"); }
        catch (const JSTypeError&) { row.set("new", "TypeError"); }
        push(empty, row);
    }
    out.set("emptyHooks", empty);
    const auto basic = nativeBasicLightingModel(), phong = nativePhongLightingModelConstructor();
    out.set("basicBase", basic.getPrototype().sameValue(ctor));
    out.set("basicPrototypeBase", basic.get("prototype").getPrototype().sameValue(prototype));
    out.set("phongBase", phong.getPrototype().sameValue(basic));
    out.set("phongCanonicalStart", phong.get("prototype").get("start").sameValue(prototype.get("start")));
    out.set("basicInstancePrototype", basic.construct().getPrototype().sameValue(basic.get("prototype")));
    out.set("phongInstancePrototype", phong.construct().getPrototype().sameValue(phong.get("prototype")));
    return out;
}
inline JSValue startCase(const std::string& mode) {
    auto state = object(), builder = object(), self = object(), first = object(), second = object();
    auto events = JSValue::array(), checks = JSValue::array(), nodes = JSValue::array();
    push(nodes, "direct-light-list");
    const auto start = nativeLightingModelConstructor().get("prototype").get("start");
    state.set("events", events); state.set("checks", checks); state.set("mode", mode); state.set("reads", 0);
    state.set("nested", false); state.set("start", start); state.set("self", self); state.set("builder", builder);
    JSPropertyDescriptor lights;
    lights.configurable = lights.enumerable = true;
    lights.getter = JSValue::function(
        +[](const JSValue& receiver, const Args&, const Args& c) -> JSValue {
            auto s = c[0]; const auto count = s.get("reads").toNumber() + 1; s.set("reads", count);
            tick(s, "get lightsNode:" + std::to_string(int(count)));
            push(s.get("checks"), receiver.sameValue(s.get("builder")));
            if ((count == 1 && s.get("mode").strictEquals("nullFirst")) ||
                (count == 2 && s.get("mode").strictEquals("nullSecond"))) return nullptr;
            return c[std::size_t(int(count) % 2 == 1 ? 1 : 2)];
        }, {state, first, second}, "get lightsNode", 0);
    builder.defineProperty("lightsNode", lights);
    const auto setup = JSValue::function(
        +[](const JSValue& receiver, const Args& args, const Args& c) -> JSValue {
            const auto s = c[0]; tick(s, "call setupLights");
            push(s.get("checks"), receiver.sameValue(c[1]) && args.size() == 2 &&
                args[0].sameValue(s.get("builder")) && args[1].sameValue(c[2]));
            reenter(s, "call setupLights"); return "ignored";
        }, {state, first, nodes}, "setupLights", 2);
    JSPropertyDescriptor setupGetter;
    setupGetter.configurable = setupGetter.enumerable = true;
    setupGetter.getter = JSValue::function(
        +[](const JSValue& receiver, const Args&, const Args& c) -> JSValue {
            tick(c[0], "get setupLights"); push(c[0].get("checks"), receiver.sameValue(c[1]));
            return c[0].get("mode").strictEquals("badSetup") ? JSValue(17) : c[2];
        }, {state, first, setup}, "get setupLights", 0);
    first.defineProperty("setupLights", setupGetter);
    const auto getNodes = JSValue::function(
        +[](const JSValue& receiver, const Args& args, const Args& c) -> JSValue {
            const auto s = c[0]; tick(s, "call getLightNodes");
            push(s.get("checks"), receiver.sameValue(c[1]) && args.size() == 1 && args[0].sameValue(s.get("builder")));
            if (s.get("mode").strictEquals("replaceSetup")) {
                auto first = c[3]; JSPropertyDescriptor d;
                d.writable = d.configurable = d.enumerable = true;
                d.value = JSValue::function(+[](const JSValue&, const Args&, const Args& captured) -> JSValue {
                    tick(captured[0], "wrong replacement"); return {};
                }, {s}, "replacement", 0);
                first.defineProperty("setupLights", d);
            }
            reenter(s, "call getLightNodes"); return c[2];
        }, {state, second, nodes, first}, "getLightNodes", 1);
    JSPropertyDescriptor nodesGetter;
    nodesGetter.configurable = nodesGetter.enumerable = true;
    nodesGetter.getter = JSValue::function(
        +[](const JSValue& receiver, const Args&, const Args& c) -> JSValue {
            tick(c[0], "get getLightNodes"); push(c[0].get("checks"), receiver.sameValue(c[1]));
            return c[0].get("mode").strictEquals("badGetNodes") ? JSValue(17) : c[2];
        }, {state, second, getNodes}, "get getLightNodes", 0);
    second.defineProperty("getLightNodes", nodesGetter);
    const auto indirect = JSValue::function(
        +[](const JSValue& receiver, const Args& args, const Args& c) -> JSValue {
            const auto s = c[0]; tick(s, "call indirect");
            push(s.get("checks"), receiver.sameValue(s.get("self")) && args.size() == 1 && args[0].sameValue(s.get("builder")));
            reenter(s, "call indirect"); return "ignored";
        }, {state}, "indirect", 1);
    JSPropertyDescriptor indirectGetter;
    indirectGetter.configurable = indirectGetter.enumerable = true;
    indirectGetter.getter = JSValue::function(
        +[](const JSValue& receiver, const Args&, const Args& c) -> JSValue {
            tick(c[0], "get indirect"); push(c[0].get("checks"), receiver.sameValue(c[0].get("self")));
            return c[0].get("mode").strictEquals("badIndirect") ? JSValue(17) : c[1];
        }, {state, indirect}, "get indirect", 0);
    self.defineProperty("indirect", indirectGetter);
    auto out = object(); out.set("mode", mode);
    try { out.set("result", start.call(self, {builder}).isUndefined() ? "undefined" : "value"); }
    catch (const JSTypeError&) { out.set("result", "TypeError"); }
    out.set("events", events); out.set("checks", checks); out.set("reads", state.get("reads"));
    return out;
}
inline JSValue all() {
    auto out = object(), cases = JSValue::array();
    out.set("metadata", metadata()); out.set("constructionAndHooks", constructionAndHooks());
    for (const auto& mode : modes()) push(cases, startCase(mode));
    out.set("startCases", cases); return out;
}
} // namespace LightingModelScenarios
