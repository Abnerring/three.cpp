#pragma once

#include "materials/nodes/MeshPhysicalNodeMaterial.h"
#include "materials/nodes/MeshStandardNodeMaterial.h"
#include "nodes/core/LightingModel.h"
#include "nodes/core/SubBuildNode.h"
#include "nodes/functions/PhysicalLightingModel.h"
#include "MeshStandardNodeMaterialFixtures.h"

namespace MeshPhysicalNodeMaterialFixtures {
using namespace MeshStandardNodeMaterialFixtures;

inline JSValue physicalSnapshot(const JSValue& material) {
    auto result = snapshot(material);
    auto scalars = result.get("scalars");
    for (const auto& key : scalars.keys()) {
        const auto value = scalars.get(key);
        if (value.type() == JSValue::Type::Number && !std::isfinite(value.number()))
            scalars.set(key, array({"number", std::isnan(value.number()) ? "NaN" : value.number() > 0 ? "Infinity" : "-Infinity"}));
    }
    return result;
}

inline JSValue metadata() {
    const auto ctor = nativeMeshPhysicalNodeMaterialConstructor(), proto = ctor.get("prototype");
    auto descriptors = array();
    for (const auto& name : proto.ownPropertyNames()) {
        if (name == "constructor") continue;
        const auto d = *proto.getOwnPropertyDescriptor(name);
        const auto fn = d.getter ? *d.getter : *d.value;
        push(descriptors, record({{"key", name}, {"getter", d.getter.has_value()},
            {"name", fn.get("name")}, {"length", fn.get("length")},
            {"source", call(fn, "toString")}, {"constructible", fn.isConstructible()},
            {"writable", d.writable ? JSValue(*d.writable) : array({"undefined"})},
            {"enumerable", d.enumerable.value_or(false)}, {"configurable", d.configurable.value_or(false)}}));
    }
    return record({{"name", ctor.get("name")}, {"length", ctor.get("length")},
        {"source", call(ctor, "toString")}, {"type", ctor.get("type")},
        {"typeSource", call(*ctor.getOwnPropertyDescriptor("type")->getter, "toString")},
        {"base", ctor.getPrototype().sameValue(nativeMeshStandardNodeMaterialConstructor())},
        {"basePrototype", proto.getPrototype().sameValue(nativeMeshStandardNodeMaterialConstructor().get("prototype"))},
        {"descriptors", descriptors}});
}
inline JSValue construct(const JSValue& request) {
    auto params = JSValue::object();
    if (request.get("configured").toBoolean()) {
        params.set("roughness", 0.23); params.set("clearcoat", 0.7);
        params.set("transmission", 0.5); params.set("sheen", 0.8);
        params.set("clearcoatNode", input("@node")); params.set("attenuationColor", 0x123456);
    }
    const auto value = nativeMeshPhysicalNodeMaterialConstructor().construct({params});
    auto flags = array();
    for (const char* key : {"useClearcoat", "useDiffuseRoughness", "useIridescence", "useSheen",
        "useAnisotropy", "useTransmission", "useDispersion", "useRetroreflection"}) push(flags, value.get(key));
    const auto clone = call(value, "clone");
    return record({{"snapshot", physicalSnapshot(value)}, {"features", flags}, {"clone", physicalSnapshot(clone)},
        {"nodeShared", clone.get("clearcoatNode").sameValue(value.get("clearcoatNode"))}});
}
inline JSValue features(const JSValue& request) {
    const auto proto = nativeMeshPhysicalNodeMaterialConstructor().get("prototype");
    auto rows = array();
    for (const auto& [name, field] : std::initializer_list<std::pair<const char*, const char*>>{
        {"useClearcoat", "clearcoat"}, {"useDiffuseRoughness", "diffuseRoughness"},
        {"useIridescence", "iridescence"}, {"useSheen", "sheen"},
        {"useAnisotropy", "anisotropy"}, {"useTransmission", "transmission"},
        {"useDispersion", "dispersion"}, {"useRetroreflection", "retroreflectivity"}}) {
        auto events = array();
        auto receiver = JSValue::object();
        JSPropertyDescriptor d; d.configurable = true;
        d.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
            push(c[0], "value"); return c[1];
        }, {events, input(request.get("value"))}, "get value", 0);
        receiver.defineProperty(field, d);
        auto coercible = JSValue::object();
        coercible.set("valueOf", JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
            push(c[0], "coerce"); if (c[1].toBoolean()) throw JSTypeError("coerce"); return c[2];
        }, {events, request.get("throws"), input(request.get("value"))}, "valueOf", 0));
        if (request.get("coerce").toBoolean()) {
            d.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
                push(c[0], "value"); return c[1];
            }, {events, coercible}, "get value", 0);
            receiver.defineProperty(field, d);
        }
        d.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
            push(c[0], "node"); return c[1];
        }, {events, input(request.get("node"))}, "get node", 0);
        receiver.defineProperty(std::string(field) + "Node", d);
        JSValue result, error(nullptr);
        try { result = proto.getOwnPropertyDescriptor(name)->getter->call(receiver); }
        catch (...) { error = failure(); }
        push(rows, record({{"key", name}, {"result", result.isUndefined() ? array({"undefined"}) : result}, {"error", error}, {"events", events}}));
    }
    return rows;
}
inline JSValue variants(const JSValue& request) {
    RestoreStack restore;
    auto material = nativeMeshPhysicalNodeMaterialConstructor().construct();
    const auto values = request.get("values");
    if (values.isObject()) for (const auto& name : values.keys()) material.set(name, input(values.get(name)));
    auto stack = nativeStackNodeConstructor().construct();
    auto events = array();
    std::vector<std::unique_ptr<RestoreProperty>> restores;
    if (request.get("observe").toBoolean()) {
        for (const char* key : {"useDiffuseRoughness", "useClearcoat", "useSheen", "useRetroreflection",
             "useIridescence", "useAnisotropy", "useTransmission", "useDispersion",
             "clearcoatNode", "clearcoatRoughnessNode", "sheenNode", "sheenRoughnessNode",
             "iridescenceNode", "iridescenceIORNode", "iridescenceThicknessNode", "anisotropyNode",
             "transmissionNode", "thicknessNode", "attenuationDistanceNode", "attenuationColorNode",
             "dispersionNode", "retroreflectivityNode", "diffuseRoughnessNode", "iorNode",
             "specularColorNode", "specularIntensityNode"}) {
            const auto value = material.get(key);
            restores.push_back(std::make_unique<RestoreProperty>(material, key));
            JSPropertyDescriptor d; d.configurable = true;
            d.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
                push(c[0], c[1]);
                if (c[2].strictEquals(c[1])) throw JSTypeError("getter");
                return c[3];
            }, {events, key, request.get("throwAt"), value}, std::string("get ") + key, 0);
            material.defineProperty(key, d);
        }
    }
    nativeSetCurrentStack(stack);
    JSValue error(nullptr), result;
    try { result = call(material, request.get("specularOnly").toBoolean() ? "setupSpecular" : "setupVariants", {JSValue::object()}); }
    catch (...) { error = failure(); }
    // ShaderNode callbacks are deferred by If. Execute both branch bodies with
    // their captured variable so their writes are covered as real TSL graphs.
    auto branches = array();
    if (request.get("branches").toBoolean() && error.isNull()) {
        try {
            auto nodes = stack.get("nodes");
            for (std::size_t i = 0; i < nodes.length(); ++i) {
                const auto n = nodes.get(JSValue(i));
                if (!n.get("isConditionalNode").strictEquals(true)) continue;
                for (const char* key : {"ifNode", "elseNode"}) {
                    const auto shader = n.get(key);
                    auto branchStack = nativeStackNodeConstructor().construct();
                    nativeSetCurrentStack(branchStack);
                    shader.get("jsFunc").call({});
                    push(branches, record({{"source", call(shader.get("jsFunc"), "toString")}, {"nodes", branchStack.get("nodes")}}));
                }
            }
        } catch (...) { error = failure(); }
    }
    return record({{"error", error}, {"returnsUndefined", result.isUndefined()}, {"events", events},
        {"graph", Graph()(record({{"nodes", stack.get("nodes")}, {"branches", branches}}))}});
}
inline JSValue model(const JSValue& request) {
    auto material = nativeMeshPhysicalNodeMaterialConstructor().construct();
    auto events = array();
    for (const char* key : {"useClearcoat", "useSheen", "useIridescence", "useAnisotropy",
         "useTransmission", "useDispersion", "useRetroreflection", "useDiffuseRoughness"}) {
        JSPropertyDescriptor d; d.configurable = true;
        d.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
            push(c[0], c[1]);
            if (c[2].strictEquals(c[1])) throw JSTypeError("flag");
            return c[3];
        }, {events, key, request.get("throwAt"), request.get("value")}, std::string("get ") + key, 0);
        material.defineProperty(key, d);
    }
    JSValue result, error(nullptr);
    try { result = call(material, "setupLightingModel"); }
    catch (...) { error = failure(); }
    return record({{"events", events}, {"error", error}, {"graph", Graph()(result)}});
}
inline JSValue setupCase(const JSValue& request) {
    auto material = nativeMeshPhysicalNodeMaterialConstructor().construct();
    auto home = nativeMeshPhysicalNodeMaterialConstructor().get("prototype");
    auto base = home.getPrototype();
    RestoreProperty restore(base, "setup");
    auto context = JSValue::object(), builder = record({{"context", context}}), events = array();
    auto sentinel = nativeTSLBaseBinding("vec3").call({}, {0.1, 0.2, 0.3});
    material.set("setupClearcoatNormal", JSValue::function(+[](const JSValue& self, const Args& a, const Args& c) -> JSValue {
        push(c[0], array({"normal", self.sameValue(c[1]), a.size(), first(a).sameValue(c[2])}));
        if (c[3].toBoolean()) throw JSTypeError("normal"); return c[4];
    }, {events, material, builder, request.get("normalThrows"), sentinel}, "normal", 0));
    base.set("setup", JSValue::function(+[](const JSValue& self, const Args& a, const Args& c) -> JSValue {
        push(c[0], array({"super", self.sameValue(c[1]), a.size(), first(a).sameValue(c[2]), c[2].get("context").get("setupClearcoatNormal").isCallable()}));
        if (c[3].toBoolean()) throw JSTypeError("super"); return "ignored";
    }, {events, material, builder, request.get("superThrows")}, "superSetup", 1));
    JSValue result, error(nullptr), callbackResult, callbackError(nullptr), callback;
    try { result = call(material, "setup", {builder}); } catch (...) { error = failure(); }
    callback = context.get("setupClearcoatNormal");
    try { callbackResult = callback.call(JSValue::object()); } catch (...) { callbackError = failure(); }
    return record({{"events", events}, {"error", error}, {"callbackError", callbackError},
        {"returnsUndefined", result.isUndefined()}, {"callbackLength", callback.get("length")},
        {"callbackSource", call(callback, "toString")}, {"graph", Graph()(callbackResult)}});
}
inline JSValue run(const JSValue& request) {
    const auto kind = request.get("kind");
    if (kind.strictEquals("metadata")) return metadata();
    if (kind.strictEquals("constructor")) return construct(request);
    if (kind.strictEquals("features")) return features(request);
    if (kind.strictEquals("variants")) return variants(request);
    if (kind.strictEquals("model")) return model(request);
    if (kind.strictEquals("setup")) return setupCase(request);
    throw std::invalid_argument("Unknown physical material scenario");
}
}
