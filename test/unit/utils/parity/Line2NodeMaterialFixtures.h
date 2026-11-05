#pragma once
#include "core/JSPromise.h"
#include "materials/nodes/Line2NodeMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "nodes/core/StackNode.h"
#include "nodes/tsl/TSLBase.h"
#include "utils.h"
#include "MeshStandardNodeMaterialFixtures.h"
#include <functional>
#include <limits>

namespace Line2NodeMaterialFixtures {
using namespace MeshStandardNodeMaterialFixtures;
inline JSValue helper(const char* name) { return nativeLine2NodeMaterialHelper(name); }
inline JSValue value(const JSValue& in) {
    if (in.strictEquals("@NaN")) return std::numeric_limits<double>::quiet_NaN();
    if (in.strictEquals("@-0")) return -0.0;
    if (in.strictEquals("@vec2")) return nativeTSLBaseBinding("vec2").call({}, {.2, .4});
    return input(in);
}
inline JSValue attempt(const std::function<JSValue()>& body) {
    try { return body(); }
    catch (const JSThrownValue& error) { return record({{"throw", error.value}}); }
    catch (const JSTypeError&) { return record({{"error", "TypeError"}}); }
    catch (const std::exception& error) { return record({{"nativeError", error.what()}}); }
}
// Retain aliases and every enumerable graph field. Only generated identifiers
// and the allocation-dependent Fn name are normalized.
class LineGraph {
    Args seen;
    JSValue objects = array();
    JSValue encode(const JSValue& in) {
        if (in.isUndefined()) return array({"undefined"});
        if (in.isCallable()) return array({"function"});
        if (in.isSymbol()) return array({"symbol"});
        if (!in.isObject()) {
            if (in.type() == JSValue::Type::Number) {
                const auto n = in.number();
                if (std::isnan(n)) return array({"number", "NaN"});
                if (!std::isfinite(n)) return array({"number", n > 0 ? "Infinity" : "-Infinity"});
                if (n == 0 && std::signbit(n)) return array({"number", "-0"});
            }
            return in;
        }
        for (unsigned i = 0; i < seen.size(); ++i) if (seen[i].sameValue(in)) return array({"ref", i});
        const auto id = seen.size(); seen.push_back(in);
        auto props = array(), row = record({{"kind", in.type() == JSValue::Type::Array ? "array" : "object"}, {"properties", props}});
        push(objects, row);
        if (in.type() == JSValue::Type::Array) row.set("length", in.length());
        if (in.get("isNode").strictEquals(true)) row.set("nodeType", in.get("type"));
        auto keys = in.keys(); std::sort(keys.begin(), keys.end());
        for (const auto& key : keys) {
            if (key == "id" || key == "uuid" || key == "_uuid" || key == "stackTrace" || key == "_cacheKey" || key == "_cacheKeyVersion") continue;
            const auto field = in.get(key);
            const auto normalized = key == "name" && in.has("inputs") && in.has("type") ? JSValue("@fn") : encode(field);
            push(props, array({key, normalized}));
        }
        return array({"ref", id});
    }
public:
    JSValue operator()(const JSValue& in) { const auto root = encode(in); return record({{"root", root}, {"objects", objects}}); }
};
inline JSValue functionMetadata(const JSValue& fn) {
    return record({{"name", fn.get("name")}, {"length", fn.get("length")},
                   {"source", call(fn, "toString")}, {"constructible", fn.isConstructible()}});
}
inline JSValue metadata() {
    const auto ctor = nativeLine2NodeMaterialConstructor(), proto = ctor.get("prototype");
    auto descriptors = array();
    for (const auto& key : proto.ownPropertyNames()) {
        if (key == "constructor") continue;
        const auto d = *proto.getOwnPropertyDescriptor(key);
        push(descriptors, record({{"key", key}, {"enumerable", d.enumerable.value_or(false)}, {"configurable", d.configurable.value_or(false)},
            {"value", d.value ? functionMetadata(*d.value) : JSValue(nullptr)},
            {"get", d.getter && !d.getter->isUndefined() ? functionMetadata(*d.getter) : JSValue(nullptr)},
            {"set", d.setter && !d.setter->isUndefined() ? functionMetadata(*d.setter) : JSValue(nullptr)}}));
    }
    auto helpers = record({});
    for (const char* key : {"trimSegmentAlpha", "closestLineToLine", "mvpLine", "alphaLine"}) {
        auto h = helper(key);
        if (!h.get("isFn").toBoolean()) while (h.get("shaderNode").isUndefined()) h = h.get("node");
        const auto shader = h.get("shaderNode");
        helpers.set(key, record({{"callback", functionMetadata(shader.get("jsFunc"))}, {"nodeType", shader.get("nodeType")},
            {"layout", shader.get("layout").isNull() ? JSValue(nullptr) : shader.get("layout").get("inputs")}}));
    }
    return record({{"class", functionMetadata(ctor)}, {"type", ctor.get("type")},
        {"typeGetter", functionMetadata(*ctor.getOwnPropertyDescriptor("type")->getter)},
        {"base", ctor.getPrototype().sameValue(nativeNodeMaterialConstructor())},
        {"prototypeBase", proto.getPrototype().sameValue(nativeNodeMaterialConstructor().get("prototype"))},
        {"descriptors", descriptors}, {"helpers", helpers}});
}
inline JSValue construct(const JSValue& request) {
    Args args;
    if (request.has("parameter")) args.push_back(value(request.get("parameter")));
    if (request.has("values")) args.push_back(request.get("values"));
    JSValue material;
    const auto result = attempt([&]() -> JSValue { material = nativeLine2NodeMaterialConstructor().construct(args); return {}; });
    auto out = record({{"result", result.isUndefined() ? array({"undefined"}) : result}});
    if (material.isObject()) {
        out.set("snapshot", snapshot(material));
        auto fields = record({});
        for (const char* key : {"vertexColors", "dashed", "worldUnits", "alphaToCoverage", "offsetNode", "dashScaleNode", "dashSizeNode", "gapSizeNode", "blending", "dashOffset", "version"}) fields.set(key, material.get(key));
        out.set("fields", LineGraph()(fields));
    }
    return out;
}
inline JSValue accessor(const JSValue& request) {
    struct RestoreConsole {
        JSValue old = ThreeUtils::getConsoleFunction();
        RestoreConsole() { ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; })); }
        ~RestoreConsole() { ThreeUtils::setConsoleFunction(old); }
    } restoreConsole;
    const auto key = request.get("key").toString();
    const auto privateKey = key == "worldUnits" ? "_useWorldUnits" : key == "dashed" ? "_useDash" : key == "alphaToCoverage" ? "_useAlphaToCoverage" : "colorNode";
    auto self = JSValue::object(), events = array(), storage = record({{"value", value(request.get("initial"))}, {"entered", false}});
    const auto d = *nativeLine2NodeMaterialConstructor().get("prototype").getOwnPropertyDescriptor(key);
    const auto observe = [&](const char* name) {
        JSPropertyDescriptor prop; prop.configurable = true;
        prop.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
            push(c[0], "get:" + c[1].toString());
            if (c[3].get("throwAt").strictEquals("get")) throw JSThrownValue{"get"};
            return c[2].get("value");
        }, {events, name, storage, request}, "", 0);
        prop.setter = JSValue::function(+[](const JSValue&, const Args& args, const Args& c) -> JSValue {
            push(c[0], array({"set:" + c[1].toString(), LineGraph()(first(args))}));
            if (c[3].get("throwAt").strictEquals(c[1])) throw JSThrownValue{c[1]};
            auto slot = c[2]; slot.set("value", first(args)); return {};
        }, {events, name, storage, request}, "", 1);
        self.defineProperty(name, prop);
    };
    observe(privateKey);
    JSPropertyDescriptor update; update.configurable = true;
    update.setter = JSValue::function(+[](const JSValue&, const Args& args, const Args& c) -> JSValue {
        push(c[0], array({"needsUpdate", first(args)}));
        if (c[1].get("throwAt").strictEquals("needsUpdate")) throw JSThrownValue{"needsUpdate"};
        return {};
    }, {events, request}, "", 1); self.defineProperty("needsUpdate", update);
    const auto before = attempt([&] { return d.getter->call(self); });
    const auto next = value(request.get("next"));
    const auto setResult = attempt([&] { return d.setter->call(self, {next}); });
    const auto after = storage.get("value");
    return LineGraph()(record({{"before", before}, {"setResult", setResult}, {"after", after}, {"events", events}}));
}
inline JSValue warningCase() {
    const auto previous = ThreeUtils::getConsoleFunction();
    struct Restore { JSValue old; ~Restore() { ThreeUtils::setConsoleFunction(old); } } restore{previous};
    auto events = array(), self = record({}), slot = record({{"value", 0}});
    ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&, const Args& args, const Args& c) -> JSValue {
        auto row = array({"console"}); for (const auto& arg : args) push(row, arg); push(c[0], row);
        throw JSThrownValue{"console"};
    }, {events}, "", 0));
    JSPropertyDescriptor d; d.configurable = true;
    d.setter = JSValue::function(+[](const JSValue&, const Args& args, const Args& c) -> JSValue {
        push(c[0], array({"color", first(args)})); auto slot = c[1]; slot.set("value", first(args)); return {};
    }, {events, slot}, "", 1); self.defineProperty("colorNode", d);
    const auto setter = *nativeLine2NodeMaterialConstructor().get("prototype").getOwnPropertyDescriptor("lineColorNode")->setter;
    const auto first = attempt([&] { return setter.call(self, {1}); });
    const auto second = attempt([&] { return setter.call(self, {2}); });
    return LineGraph()(record({{"first", first}, {"second", second}, {"value", slot.get("value")}, {"events", events}}));
}
inline JSValue shaderBody(const char* name) {
    auto h = helper(name);
    if (!h.get("isFn").toBoolean()) while (h.get("shaderNode").isUndefined()) h = h.get("node");
    return h.get("shaderNode").get("jsFunc");
}
inline JSValue graphCase(const JSValue& request) {
    RestoreStack restore;
    auto stack = nativeStackNodeConstructor().construct(), events = array(), material = JSValue::object(), renderer = record({});
    for (const char* key : {"_useDash", "_useWorldUnits", "_useAlphaToCoverage"}) material.set(key, request.get(key));
    for (const char* key : {"dashScaleNode", "offsetNode", "dashSizeNode", "gapSizeNode"}) {
        const auto raw = value(request.get(key));
        JSPropertyDescriptor d; d.configurable = true;
        d.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
            push(c[0], c[1]); if (c[2].get("throwAt").strictEquals(c[1])) throw JSThrownValue{c[1]}; return c[3];
        }, {events, key, request, raw}, "", 0); material.defineProperty(key, d);
    }
    renderer.set("currentSamples", request.has("samples") ? value(request.get("samples")) : JSValue(4));
    auto input = record({{"material", material}, {"renderer", renderer}});
    const auto name = request.get("helper").toString();
    if (name == "trimSegmentAlpha") input = record({{"start", nativeTSLBaseBinding("vec4").call({}, {1, 2, -3, 1})}, {"end", nativeTSLBaseBinding("vec4").call({}, {2, 3, 4, 1})}});
    if (name == "closestLineToLine") {
        input = record({});
        for (const char* key : {"p1", "p2", "p3", "p4"}) input.set(key, nativeTSLBaseBinding("vec3").call({}, {.2, .4, .8}));
    }
    nativeSetCurrentStack(stack);
    const auto result = attempt([&] { return shaderBody(name.c_str()).call({}, {input}); });
    auto branches = array(); Args seen;
    std::function<void(const JSValue&)> walk = [&](const JSValue& node) {
        if (!node.isObject() || node.isCallable()) return;
        for (const auto& old : seen) if (old.sameValue(node)) return;
        seen.push_back(node);
        if (node.get("type").strictEquals("ConditionalNode")) {
            for (const char* key : {"ifNode", "elseNode"}) {
                const auto shader = node.get(key);
                if (shader.isObject() && shader.get("jsFunc").isCallable()) {
                    auto branchStack = nativeStackNodeConstructor().construct(); nativeSetCurrentStack(branchStack);
                    const auto value = attempt([&] { return shader.get("jsFunc").call(); });
                    push(branches, record({{"source", call(shader.get("jsFunc"), "toString")}, {"result", value}, {"nodes", branchStack.get("nodes")}}));
                    walk(branchStack.get("nodes")); nativeSetCurrentStack(stack);
                }
            }
        }
        for (const auto& key : node.keys()) if (key != "stackTrace") walk(node.get(key));
    };
    if (!request.get("skipBranches").toBoolean()) walk(stack.get("nodes"));
    return LineGraph()(record({{"result", result}, {"nodes", stack.get("nodes")}, {"branches", branches}, {"events", events}}));
}
inline JSValue setupCase(const JSValue& request) {
    RestoreStack restore;
    auto material = nativeLine2NodeMaterialConstructor().construct(), stack = nativeStackNodeConstructor().construct(), events = array();
    const auto proto = nativeLine2NodeMaterialConstructor().get("prototype"), base = proto.getPrototype();
    auto home = proto; RestoreProperty oldDiffuse(base, "setupDiffuseColor"), oldPosition(base, "setupPosition");
    auto builder = record({}), geometry = record({}); builder.set("geometry", geometry);
    for (const char* key : {"vertexColors", "transparent"}) material.set(key, value(request.get(key)));
    geometry.set("hasAttribute", JSValue::function(+[](const JSValue&, const Args& a, const Args& c) -> JSValue {
        push(c[0], array({"hasAttribute", first(a)})); if (c[1].get("throwAt").strictEquals("hasAttribute")) throw JSThrownValue{"hasAttribute"}; return c[1].get("attribute");
    }, {events, request}, "", 1));
    builder.set("needsPreviousData", JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
        push(c[0], "needsPreviousData"); if (c[1].get("throwAt").strictEquals("needsPreviousData")) throw JSThrownValue{"needsPreviousData"}; return c[1].get("previous");
    }, {events, request}, "", 0));
    const auto install = [&](const char* key) {
        auto mutableBase = base;
        mutableBase.set(key, JSValue::function(+[](const JSValue& self, const Args& a, const Args& c) -> JSValue {
            push(c[0], array({c[1], self.sameValue(c[2]), a.size(), first(a).sameValue(c[3])}));
            if (c[4].get("throwAt").strictEquals("super")) throw JSThrownValue{"super"}; return c[5];
        }, {events, key, material, builder, request, helper("positionLocal")}, key, 1));
    };
    install("setupDiffuseColor"); install("setupPosition"); nativeSetCurrentStack(stack);
    const auto result = attempt([&] { return call(material, request.get("method").toString().c_str(), {builder}); });
    return LineGraph()(record({{"result", result}, {"nodes", stack.get("nodes")}, {"events", events}}));
}
inline JSValue run(const JSValue& request) {
    const auto kind = request.get("kind");
    if (kind.strictEquals("metadata")) return metadata();
    if (kind.strictEquals("constructor")) return construct(request);
    if (kind.strictEquals("accessor")) return accessor(request);
    if (kind.strictEquals("warning")) return warningCase();
    if (kind.strictEquals("graph")) return graphCase(request);
    if (kind.strictEquals("setup")) return setupCase(request);
    throw std::invalid_argument("Unknown Line2 scenario");
}
} // namespace Line2NodeMaterialFixtures
