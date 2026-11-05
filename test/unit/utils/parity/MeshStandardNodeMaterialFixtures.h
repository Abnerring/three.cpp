#pragma once

#include "materials/nodes/MeshStandardNodeMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/StackNode.h"
#include "nodes/functions/material/getRoughness.h"
#include "nodes/tsl/TSLBase.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace MeshStandardNodeMaterialFixtures {
using Args = std::vector<JSValue>;
inline JSValue call(const JSValue& self, const char* key, const Args& args = {}) {
    return self.get(key).call(self, args);
}
inline JSValue array(const Args& values = {}) {
    auto out = JSValue::array();
    for (const auto& value : values) call(out, "push", {value});
    return out;
}
inline void push(const JSValue& out, const JSValue& value) { call(out, "push", {value}); }
inline JSValue record(std::initializer_list<std::pair<const char*, JSValue>> values) {
    auto out = JSValue::object();
    for (const auto& [key, value] : values) out.set(key, value);
    return out;
}
inline JSValue first(const Args& args) { return args.empty() ? JSValue{} : args[0]; }
inline JSValue failure() {
    try { throw; }
    catch (const JSTypeError&) { return "TypeError"; }
    catch (const JSRangeError&) { return "RangeError"; }
    catch (const std::exception& error) { return std::string("NativeError:") + error.what(); }
}
inline JSValue input(const JSValue& value) {
    if (value.strictEquals("@undefined")) return {};
    if (value.strictEquals("@node")) return nativeTSLBaseBinding("float").call({}, {0.37});
    if (value.strictEquals("@vec3")) return nativeTSLBaseBinding("vec3").call({}, {0.2, 0.4, 0.6});
    return value;
}
inline void checkpoint(const JSValue& state, const std::string& label) {
    push(state.get("events"), label);
    if (state.get("request").get("throwAt").strictEquals(label)) throw JSTypeError(label);
}
struct RestoreProperty {
    JSValue object;
    std::string key;
    std::optional<JSPropertyDescriptor> old;
    RestoreProperty(JSValue object, const std::string& key)
        : object(object), key(key), old(object.getOwnPropertyDescriptor(key)) {}
    ~RestoreProperty() { if (old) object.defineProperty(key, *old); else object.erase(key); }
};
struct RestoreStack {
    JSValue old = nativeGetCurrentStack();
    ~RestoreStack() { nativeSetCurrentStack(old); }
};
// Keep all enumerable graph state and aliasing, omitting only generated IDs,
// UUIDs and diagnostic traces. In particular this retains rawInputs, shared
// property references, operators, constants and shader-call structure.
class Graph {
    Args seen;
    JSValue objects = array();
    JSValue encode(const JSValue& value) {
        if (value.isUndefined()) return array({"undefined"});
        if (value.isCallable()) return array({"function"});
        if (value.isSymbol()) return array({"symbol"});
        if (value.type() == JSValue::Type::Number) {
            if (std::isnan(value.number())) return array({"number", "NaN"});
            if (!std::isfinite(value.number())) return array({"number", value.number() > 0 ? "Infinity" : "-Infinity"});
            if (value.number() == 0 && std::signbit(value.number())) return array({"number", "-0"});
        }
        if (!value.isObject()) return value;
        for (std::size_t i = 0; i < seen.size(); ++i)
            if (seen[i].sameValue(value)) return array({"ref", i});
        const auto id = seen.size();
        seen.push_back(value);
        auto props = array(), object = record({{"kind", value.type() == JSValue::Type::Array ? "array" : "object"}, {"properties", props}});
        push(objects, object);
        if (value.type() == JSValue::Type::Array) object.set("length", value.length());
        if (value.get("isNode").strictEquals(true)) object.set("nodeType", value.get("type"));
        for (const auto& key : value.keys()) {
            if (key == "id" || key == "uuid" || key == "_uuid" || key == "stackTrace" ||
                key == "_cacheKey" || key == "_cacheKeyVersion") continue;
            push(props, array({key, encode(value.get(key))}));
        }
        return array({"ref", id});
    }
  public:
    JSValue operator()(const JSValue& value) {
        const auto root = encode(value);
        return record({{"root", root}, {"objects", objects}});
    }
};
inline JSValue snapshot(const JSValue& material) {
    auto names = material.ownPropertyNames();
    std::sort(names.begin(), names.end());
    auto list = array(), kinds = JSValue::object(), scalars = JSValue::object();
    for (const auto& name : names) {
        if (name == "uuid") continue;
        push(list, name);
        const auto value = material.get(name);
        const char* kind = value.isNull() ? "null" : value.isUndefined() ? "undefined" :
            value.isCallable() ? "function" : value.type() == JSValue::Type::Number ? "number" :
            value.type() == JSValue::Type::Boolean ? "boolean" : value.type() == JSValue::Type::String ? "string" : "object";
        kinds.set(name, kind);
        if (name != "id" && (value.type() == JSValue::Type::Number || value.type() == JSValue::Type::Boolean ||
                            value.type() == JSValue::Type::String)) scalars.set(name, value);
    }
    return record({{"names", list}, {"kinds", kinds}, {"scalars", scalars}});
}
inline JSValue metadata() {
    const auto ctor = nativeMeshStandardNodeMaterialConstructor(), proto = ctor.get("prototype");
    auto methods = array();
    for (const char* name : {"setupEnvironment", "setupLightingModel", "setupSpecular", "setupVariants"}) {
        const auto d = *proto.getOwnPropertyDescriptor(name);
        const auto fn = *d.value;
        push(methods, record({{"key", name}, {"name", fn.get("name")}, {"length", fn.get("length")},
            {"constructible", fn.isConstructible()}, {"writable", d.writable.value_or(false)}, {"enumerable", d.enumerable.value_or(false)},
            {"configurable", d.configurable.value_or(false)}, {"source", call(fn, "toString")}}));
    }
    const auto d = *ctor.getOwnPropertyDescriptor("type");
    const auto getter = *d.getter;
    return record({{"name", ctor.get("name")}, {"length", ctor.get("length")}, {"type", ctor.get("type")},
        {"source", call(ctor, "toString")}, {"base", ctor.getPrototype().sameValue(nativeNodeMaterialConstructor())},
        {"basePrototype", proto.getPrototype().sameValue(nativeNodeMaterialConstructor().get("prototype"))},
        {"prototypeKeys", array([&] { Args a; for (const auto& k : proto.ownPropertyNames()) a.push_back(k); return a; }())},
        {"typeGetter", record({{"name", getter.get("name")}, {"length", getter.get("length")},
            {"source", call(getter, "toString")}, {"enumerable", d.enumerable.value_or(false)}, {"configurable", d.configurable.value_or(false)},
            {"setter", d.setter && !d.setter->isUndefined()}, {"constructible", getter.isConstructible()}})}, {"methods", methods}});
}
inline JSValue constructorCase(const JSValue& request) {
    const auto ctor = nativeMeshStandardNodeMaterialConstructor();
    auto params = JSValue::object();
    params.set("roughness", 0.23); params.set("metalness", 0.81); params.set("color", 0x2468ac);
    params.set("emissive", 0x102030); params.set("emissiveIntensity", 2.5);
    params.set("emissiveNode", input("@vec3")); params.set("metalnessNode", input("@node"));
    const auto material = request.get("parameters").toBoolean() ? ctor.construct({params}) : ctor.construct();
    const auto other = ctor.construct();
    auto out = record({{"snapshot", snapshot(material)},
        {"color", array({material.get("color").get("r"), material.get("color").get("g"), material.get("color").get("b")})},
        {"emissive", array({material.get("emissive").get("r"), material.get("emissive").get("g"), material.get("emissive").get("b")})},
        {"independentColor", !material.get("color").sameValue(other.get("color"))},
        {"independentNormalScale", !material.get("normalScale").sameValue(other.get("normalScale"))},
        {"independentEmissive", !material.get("emissive").sameValue(other.get("emissive"))},
        {"independentRotation", !material.get("envMapRotation").sameValue(other.get("envMapRotation"))},
        {"metalnessInput", material.get("metalnessNode").sameValue(params.get("metalnessNode"))},
        {"emissiveInput", material.get("emissiveNode").sameValue(params.get("emissiveNode"))}});
    return out;
}
inline JSValue constructorHooks(const JSValue& request) {
    const auto ctor = nativeMeshStandardNodeMaterialConstructor();
    auto state = record({{"request", request}, {"events", array()}, {"defaults", nullptr}, {"nested", false}});
    auto derived = JSValue::builtinConstructor(+[](const JSValue&, const Args&, const Args&) -> JSValue {
        return JSValue::object();
    }, {}, "Derived", 0);
    auto proto = derived.get("prototype"); proto.setPrototype(ctor.get("prototype"));
    derived.setPrototype(ctor);
    for (const char* name : {"setDefaultValues", "setValues"}) {
        proto.set(name, JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            const auto state = captures[0];
            const auto label = std::string(captures[1].string());
            checkpoint(state, label + (state.get("nested").toBoolean() ? ":nested" : ":outer"));
            push(state.get("events"), record({{"flag", self.get("isMeshStandardNodeMaterial")}, {"lights", self.get("lights")},
                {"emissiveNull", self.get("emissiveNode").isNull()}, {"metalnessNull", self.get("metalnessNode").isNull()},
                {"roughnessNull", self.get("roughnessNode").isNull()}}));
            if (label == "setDefaultValues") {
                const auto value = first(args);
                if (state.get("defaults").isNull()) { auto s = state; s.set("defaults", value); }
                push(state.get("events"), value.sameValue(state.get("defaults")));
                if (state.get("request").get("reenter").toBoolean() && !state.get("nested").toBoolean()) {
                    auto s = state; s.set("nested", true);
                    nativeMeshStandardNodeMaterialConstructor().construct({"nested"}, captures[2]);
                    s.set("nested", false);
                }
            } else push(state.get("events"), first(args));
            return 99; // Both hook return values are ignored by the constructor.
        }, {state, name, derived}, name, 1));
    }
    JSValue error(nullptr), result;
    try { result = ctor.construct({"outer"}, derived); } catch (...) { error = failure(); }
    return record({{"error", error}, {"events", state.get("events")},
        {"derived", result.isObject() && result.getPrototype().sameValue(proto)}});
}
inline JSValue environmentCase(const JSValue& request) {
    const auto ctor = nativeMeshStandardNodeMaterialConstructor(), proto = ctor.get("prototype");
    const auto homeBase = proto.getPrototype();
    auto material = ctor.construct();
    auto state = record({{"request", request}, {"events", array()}, {"reads", 0}, {"nested", false}});
    auto builder = JSValue::object();
    const auto fallback = input(request.get("fallback"));
    JSPropertyDescriptor descriptor; descriptor.configurable = descriptor.enumerable = true;
    descriptor.getter = JSValue::function(+[](const JSValue& self, const Args&, const Args& c) -> JSValue {
        auto s = c[0];
        const auto reads = s.get("reads").toNumber() + 1; s.set("reads", reads);
        checkpoint(s, "environment:" + std::to_string(static_cast<unsigned>(reads)));
        push(s.get("events"), self.sameValue(c[1]));
        if (reads == 2 && s.get("request").has("second")) return input(s.get("request").get("second"));
        return c[2];
    }, {state, builder, fallback}, "get environmentNode", 0);
    builder.defineProperty("environmentNode", descriptor);
    JSValue result, error(nullptr);
    if (request.get("realBase").toBoolean()) {
        material.set("envNode", input(request.get("base")));
        if (request.get("map").toBoolean()) material.set("envMap", record({{"isCubeTexture", request.get("cube")}}));
        try { result = call(material, "setupEnvironment", {builder}); } catch (...) { error = failure(); }
    } else {
        auto bridge = JSValue::object(homeBase);
        descriptor.getter = JSValue::function(+[](const JSValue& self, const Args&, const Args& c) -> JSValue {
            checkpoint(c[0], "super:get"); push(c[0].get("events"), self.sameValue(c[1]));
            return JSValue::function(+[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                checkpoint(c[0], "super:call");
                push(c[0].get("events"), array({self.sameValue(c[1]), args.size(), first(args).sameValue(c[2])}));
                if (c[0].get("request").get("reenter").toBoolean() && !c[0].get("nested").toBoolean()) {
                    auto s = c[0]; s.set("nested", true);
                    const auto value = c[3].call(c[1], {c[2]});
                    push(s.get("events"), value.isNull() ? JSValue("nested:null") : JSValue("nested:environment"));
                    s.set("nested", false);
                }
                return input(c[0].get("request").get("base"));
            }, c, "superEnvironment", 1);
        }, {state, material, builder, proto.get("setupEnvironment")}, "get setupEnvironment", 0);
        bridge.defineProperty("setupEnvironment", descriptor);
        auto home = proto; home.setPrototype(bridge);
        try { result = call(material, "setupEnvironment", {builder}); } catch (...) { error = failure(); }
        home.setPrototype(homeBase);
    }
    auto out = record({{"error", error}, {"events", state.get("events")}, {"isNull", result.isNull()}});
    if (error.isNull() && !result.isNull()) {
        out.set("type", result.get("type"));
        out.set("envIsFallback", result.get("envNode").sameValue(fallback));
        out.set("env", Graph()(result.get("envNode")));
    }
    return out;
}
inline void sequenceProperty(const JSValue& state, JSValue material, const char* key, const JSValue& sequence) {
    auto slot = record({{"at", 0}});
    JSPropertyDescriptor d; d.configurable = d.enumerable = true;
    d.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
        auto index = c[1]; const auto at = index.get("at").toNumber(); index.set("at", at + 1);
        checkpoint(c[0], "get:" + c[2].string() + ":" + std::to_string(static_cast<unsigned>(at + 1)));
        return c[3].get(JSValue(std::min(at, double(c[3].length() - 1))));
    }, {state, slot, key, sequence}, (std::string("get ") + key).c_str(), 0);
    material.defineProperty(key, d);
}
inline JSValue variantsCase(const JSValue& request) {
    auto material = nativeMeshStandardNodeMaterialConstructor().construct();
    const auto stack = nativeStackNodeConstructor().construct();
    RestoreStack restoreStack;
    auto state = record({{"request", request}, {"events", array()}, {"nested", false}});
    for (const char* key : {"metalnessNode", "roughnessNode"}) {
        auto sequence = array();
        const auto values = request.get(key);
        if (values.isUndefined()) push(sequence, nullptr);
        else for (unsigned i = 0; i < values.length(); ++i) push(sequence, input(values.get(JSValue(i))));
        sequenceProperty(state, material, key, sequence);
    }
    const auto originalSpecular = material.get("setupSpecular");
    JSPropertyDescriptor d; d.configurable = d.enumerable = true;
    d.getter = JSValue::function(+[](const JSValue& self, const Args&, const Args& c) -> JSValue {
        checkpoint(c[0], "get:setupSpecular"); push(c[0].get("events"), self.sameValue(c[1]));
        return JSValue::function(+[](const JSValue& self, const Args& a, const Args& c) -> JSValue {
            checkpoint(c[0], "call:setupSpecular");
            push(c[0].get("events"), array({self.sameValue(c[1]), a.size()}));
            if (c[0].get("request").get("reenter").toBoolean() && !c[0].get("nested").toBoolean()) {
                auto state = c[0]; state.set("nested", true);
                const auto inner = nativeMeshStandardNodeMaterialConstructor().construct();
                call(inner, "setupVariants"); state.set("nested", false);
            }
            if (!c[0].get("request").get("overrideSpecular").toBoolean()) return c[2].call(self);
            return {};
        }, c, "specularHook", 0);
    }, {state, material, originalSpecular}, "get setupSpecular", 0);
    material.defineProperty("setupSpecular", d);
    std::vector<std::unique_ptr<RestoreProperty>> restores;
    if (request.get("observeAssign").toBoolean()) {
        for (const char* key : {"metalness", "roughness", "specularColor", "specularColorBlended", "specularF90", "diffuseContribution"}) {
            auto property = nativePropertyNodeBinding(key);
            const auto original = property.get("assign");
            restores.push_back(std::make_unique<RestoreProperty>(property, "assign"));
            JSPropertyDescriptor d; d.configurable = true;
            d.getter = JSValue::function(+[](const JSValue& self, const Args&, const Args& c) -> JSValue {
                checkpoint(c[0], "get:assign:" + c[1].string());
                push(c[0].get("events"), self.sameValue(c[2]));
                return JSValue::function(+[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                    checkpoint(c[0], "call:assign:" + c[1].string());
                    push(c[0].get("events"), array({self.sameValue(c[2]), args.size()}));
                    return c[3].call(self, args);
                }, c, "assignHook", 1);
            }, {state, key, property, original}, "get assign", 0);
            property.defineProperty("assign", d);
        }
    }
    nativeSetCurrentStack(stack);
    JSValue error(nullptr), result;
    try { result = call(material, request.get("specularOnly").toBoolean() ? "setupSpecular" : "setupVariants"); }
    catch (...) { error = failure(); }
    JSValue roughnessBody;
    if (request.get("roughnessBody").toBoolean() && error.isNull()) {
        try {
            for (unsigned i = 0; i < stack.get("nodes").length(); ++i) {
                const auto n = stack.get("nodes").get(JSValue(i));
                if (!n.get("targetNode").isObject() || !n.get("targetNode").get("name").strictEquals("Roughness")) continue;
                auto source = n.get("sourceNode");
                while (source.get("shaderNode").isUndefined()) source = source.get("node");
                roughnessBody = source.get("shaderNode").get("jsFunc").call({}, {source.get("rawInputs").get("0")});
                break;
            }
        } catch (...) { error = failure(); }
    }
    restores.clear();
    return record({{"error", error}, {"returnsUndefined", result.isUndefined()}, {"events", state.get("events")},
        {"graph", Graph()(record({{"nodes", stack.get("nodes")}, {"roughnessBody", roughnessBody}}))}});
}
inline JSValue modelCase() {
    const auto material = nativeMeshStandardNodeMaterialConstructor().construct();
    const auto a = call(material, "setupLightingModel"), b = call(material, "setupLightingModel");
    return record({{"distinct", !a.sameValue(b)}, {"prototypeName", a.get("constructor").get("name")},
        {"snapshot", Graph()(a)}});
}
inline JSValue run(const JSValue& request) {
    const auto kind = request.get("kind");
    if (kind.strictEquals("metadata")) return metadata();
    if (kind.strictEquals("constructor")) return constructorCase(request);
    if (kind.strictEquals("constructorHooks")) return constructorHooks(request);
    if (kind.strictEquals("environment")) return environmentCase(request);
    if (kind.strictEquals("variants")) return variantsCase(request);
    if (kind.strictEquals("model")) return modelCase();
    throw std::invalid_argument("Unknown MeshStandardNodeMaterial fixture");
}
} // namespace MeshStandardNodeMaterialFixtures
