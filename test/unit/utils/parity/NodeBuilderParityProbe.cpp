#include "nodes/core/NodeBuilder.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeCache.h"
#include "nodes/core/NodeAttribute.h"
#include "nodes/core/NodeUniform.h"
#include "nodes/core/NodeVar.h"
#include "nodes/core/NodeVarying.h"
#include "nodes/core/NodeCode.h"
#include "nodes/core/StructType.h"
#include "core/JSSet.h"
#include "core/NativeArray.h"
#include "utils.h"
#include <cmath>
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
using A = JsonValue::Array;
using O = JsonValue::Object;
JSValue call(const JSValue& self, const char* name, const Args& args = {}) { return self.get(name).call(self, args); }
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "#undefined";
    if (value.isCallable()) return "#function";
    if (value.isSymbol()) return "#symbol";
    if (value.isObject()) return "#object";
    if (value.type() == JSValue::Type::Number && !std::isfinite(value.toNumber())) return value.toString();
    return *value.toJSON();
}
JsonValue snapshot(const JSValue& value) {
    O result;
    for (const auto& key : value.ownPropertyNames()) {
        const auto d = *value.getOwnPropertyDescriptor(key);
        O row{{"enumerable", d.enumerable.value_or(false)}, {"configurable", d.configurable.value_or(false)}};
        if (d.value) { row["value"] = encode(*d.value); row["writable"] = d.writable.value_or(false); }
        else { row["get"] = d.getter && d.getter->isCallable(); row["set"] = d.setter && d.setter->isCallable(); }
        result[key] = row;
    }
    return result;
}
void emit(O value) { std::cout << JsonValue(value).dump() << '\n'; }
template<class F> JsonValue attempt(F function) {
    try { return O{{"value", encode(function())}}; }
    catch (const JSTypeError&) { return O{{"error", "TypeError"}}; }
    catch (const JSRangeError&) { return O{{"error", "RangeError"}}; }
    catch (const std::exception&) { return O{{"error", "Error"}}; }
}
JSValue renderer() {
    auto r = JSValue::object(), debug = JSValue::object(), diagnostics = JSValue::object();
    diagnostics.set("keywords", false); debug.set("diagnostics", diagnostics); r.set("debug", debug); return r;
}
JSValue builder() { auto b = NodeBuilder(nullptr, renderer(), nullptr).object(); b.set("shaderStage", "fragment"); return b; }
JSValue node() {
    auto n = nativeNodeConstructor().construct({"float"});
    n.set("getHash", JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue("node-hash"); }, {}, "getHash", 1));
    n.set("generate", JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue("4.0"); }, {}, "generate", 1));
    return n;
}
void metadata() {
    const auto b = builder(), ctor = NodeBuilder::constructor(), proto = NodeBuilder::prototype();
    A keys; for (const auto& key : b.ownPropertyNames()) keys.push_back(key);
    O methods;
    for (const auto* method : {"getDataFromNode", "getNodeProperties", "getVarFromNode", "getVaryingFromNode", "flowStagesNode", "format", "getTypeFromLength", "getSubBuildProperty", "build", "generateConst"}) {
        const auto fn = proto.get(method); methods[method] = O{{"name", encode(fn.get("name"))}, {"length", encode(fn.get("length"))}, {"construct", attempt([&] { return fn.construct(); })}};
    }
    emit({{"kind", "metadata"}, {"constructor", snapshot(ctor)}, {"keys", keys}, {"instance", snapshot(b)}, {"methods", methods},
        {"prototype", b.getPrototype().strictEquals(proto)}, {"constructorIdentity", proto.get("constructor").strictEquals(ctor)},
        {"globalCache", b.get("cache").strictEquals(b.get("globalCache"))}, {"stack", encode(b.get("stack").get("isStackNode"))},
        {"call", attempt([&] { return ctor.call(); })}});
    const std::vector<std::pair<const char*, JSValue>> classes{{"NodeVar", nativeNodeVarConstructor()}, {"NodeAttribute", nativeNodeAttributeConstructor()},
        {"NodeUniform", nativeNodeUniformConstructor()}, {"NodeVarying", nativeNodeVaryingConstructor()}, {"NodeCode", nativeNodeCodeConstructor()}, {"StructType", nativeStructTypeConstructor()}};
    for (const auto& [name, constructor] : classes) {
        const auto instance = constructor.construct({"declaration", "vec3", node()});
        emit({{"kind", "dependency"}, {"name", name}, {"instance", snapshot(instance)}, {"constructor", snapshot(constructor)}, {"prototype", snapshot(constructor.get("prototype"))},
            {"parent", name == std::string("NodeVarying") ? constructor.get("prototype").getPrototype().strictEquals(nativeNodeVarConstructor().get("prototype")) : constructor.get("prototype").getPrototype().strictEquals(JSValue::intrinsicObjectConstructor().get("prototype"))}});
    }
}
const std::vector<std::string> types{"float", "bool", "int", "uint", "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "uvec2", "uvec3", "uvec4", "bvec2", "bvec3", "bvec4", "mat2", "mat3", "mat4", "color", "texture", "cubeTexture", "depthTexture", "texture3D", "void", "property", "unknown", "prefix_ivec3_suffix"};
void typesAndFormat() {
    const auto b = builder();
    for (const auto& type : types) {
        O row{{"kind", "types"}, {"type", type}};
        for (const auto* method : {"getType", "isScalar", "isVector", "isMatrix", "isReference", "getElementType", "getComponentType", "getVectorType", "isInteger", "getTypeLength", "getVectorFromMatrix", "getIntegerType", "generateConst"})
            row[method] = attempt([&] { return call(b, method, {type}); });
        emit(row);
    }
    for (const auto& from : types) for (const auto& to : types)
        emit({{"kind", "format"}, {"from", from}, {"to", to}, {"result", attempt([&] { return call(b, "format", {"sample", from, to}); })}});
    for (int length : {0, 1, 2, 3, 4, 9, 16, 5}) for (const auto* component : {"float", "bool", "int", "uint", "mat2"})
        emit({{"kind", "length"}, {"length", length}, {"component", component}, {"result", attempt([&] { return call(b, "getTypeFromLength", {length, component}); })}});
    for (unsigned kind = 0; kind < 9; ++kind)
        emit({{"kind", "arrayType"}, {"index", kind}, {"result", attempt([&] { return call(b, "getTypeFromArray", {NativeArray_detail::constructor(kind).construct({0})}); })}});
}
void dataAndUsage() {
    auto b = builder(), n = node(), global = node(); global.set("global", true);
    const auto base = call(b, "getDataFromNode", {n}), globalData = call(b, "getDataFromNode", {global});
    const auto properties = call(b, "getNodeProperties", {n});
    auto subBuilds = createNativeSet(); call(subBuilds, "add", {"outer"}); call(subBuilds, "add", {"inner"});
    auto any = call(b, "getDataFromNode", {n, "any"}); any.set("subBuilds", subBuilds);
    call(b, "addSubBuild", {"outer"}); const auto outer = call(b, "getDataFromNode", {n});
    call(b, "addSubBuild", {"inner"}); const auto inner = call(b, "getDataFromNode", {n});
    A rows{!base.strictEquals(outer), !outer.strictEquals(inner), outer.get("subBuilds").strictEquals(subBuilds), encode(call(b, "getSubBuildProperty", {"variable", subBuilds}))};
    rows.push_back(encode(call(b, "removeSubBuild"))); rows.push_back(call(b, "getDataFromNode", {n}).strictEquals(outer));
    rows.push_back(encode(call(b, "removeSubBuild"))); rows.push_back(call(b, "getDataFromNode", {n}).strictEquals(base));
    rows.push_back(call(b, "getNodeProperties", {n}).strictEquals(properties));
    rows.push_back(encode(call(b, "increaseUsage", {n}))); rows.push_back(encode(call(b, "hasWriteUsage", {n})));
    auto context = b.get("context"); context.set("assign", true);
    rows.push_back(encode(call(b, "increaseUsage", {n}))); rows.push_back(encode(call(b, "hasWriteUsage", {n})));
    rows.push_back(encode(base.get("readUsageCount"))); rows.push_back(encode(base.get("writeUsageCount")));
    const auto previous = b.get("cache"); const auto nested = call(b, "getCacheFromNode", {n});
    rows.push_back(nested.get("parent").strictEquals(previous)); call(b, "setCache", {nested});
    rows.push_back(call(b, "getDataFromNode", {n}).strictEquals(base));
    call(b, "setCache", {nativeNodeCacheConstructor().construct()});
    rows.push_back(!call(b, "getDataFromNode", {n}).strictEquals(base)); rows.push_back(call(b, "getDataFromNode", {global}).strictEquals(globalData));
    emit({{"kind", "data"}, {"rows", rows}});
}
void declarationAndFlow() {
    auto b = builder(), n = node(), other = node(); A rows;
    auto v = call(b, "getVarFromNode", {n, "shared", "float"});
    rows.push_back(call(b, "getVarFromNode", {n}).strictEquals(v));
    rows.push_back(encode(call(b, "getVarFromNode", {other, "shared", "vec3"}).get("name")));
    rows.push_back(encode(call(b, "getVars", {"fragment"})));
    auto u = call(b, "getUniformFromNode", {n, "float"}); u.set("value", 17); rows.push_back(encode(n.get("value"))); rows.push_back(u.get("node").strictEquals(n));
    const auto varying = call(b, "getVaryingFromNode", {n}); rows.push_back(encode(varying.get("isNodeVar"))); rows.push_back(encode(varying.get("interpolationType")));
    call(b, "addLineFlowCode", {"line"}); call(b, "addLineFlowCode", {"already;  "}); call(b, "addLineFlowCode", {""}); call(b, "addFlowTab"); call(b, "addLineFlowCode", {"nested"}); call(b, "removeFlowTab");
    rows.push_back(encode(b.get("flow").get("code"))); rows.push_back(encode(b.get("tab")));
    const auto code = call(b, "getCodeFromNode", {n, "code"}); auto codeRef = code; codeRef.set("code", "functionCode"); rows.push_back(encode(call(b, "getCodes", {"fragment"})));
    const auto original = b.get("flow"), cache = b.get("cache"), stack = b.get("stack");
    b.set("buildStage", "before"); const auto flow = call(b, "flowStagesNode", {n, "vec3"});
    rows.push_back(encode(flow.get("result"))); rows.push_back(encode(flow.get("vars")));
    rows.push_back(b.get("flow").strictEquals(original)); rows.push_back(b.get("cache").strictEquals(cache)); rows.push_back(b.get("stack").strictEquals(stack)); rows.push_back(encode(b.get("buildStage")));
    b.set("buildStage", "generate"); const auto cross = call(b, "flowNodeFromShaderStage", {"vertex", n, "float", "position"});
    rows.push_back(encode(cross.get("result"))); rows.push_back(encode(b.get("flowCode").get("vertex"))); rows.push_back(encode(b.get("shaderStage")));
    emit({{"kind", "declarationFlow"}, {"rows", rows}});
}
void nodeBuild() {
    auto b = builder(), n = node(); A rows;
    b.set("buildStage", "setup"); call(n, "build", {b, "float"});
    b.set("buildStage", "analyze"); call(n, "build", {b, "float"}); call(n, "build", {b, "float"});
    b.set("buildStage", "generate"); rows.push_back(encode(call(n, "build", {b, "float"}))); rows.push_back(encode(b.get("flow").get("code")));
    rows.push_back(encode(call(n, "build", {b, "vec3"}))); rows.push_back(encode(b.get("flow").get("code"))); rows.push_back(b.get("chaining").length());
    emit({{"kind", "nodeBuild"}, {"rows", rows}});
}
void dynamic() {
    auto b = builder(), n = node(), events = JSValue::array(); auto state = JSValue::object(); state.set("events", events);
    b.set("getDataFromNode", JSValue::function([](const JSValue&, const Args& args, const Args& captures) {
        call(captures[0].get("events"), "push", {args.size() > 1 ? args[1] : JSValue("#omitted")}); return captures[0].get("data");
    }, {state}, "replacement", 1));
    auto data = JSValue::object(); state.set("data", data); call(b, "increaseUsage", {n});
    auto assign = b.get("context"); assign.set("assign", true); call(b, "increaseUsage", {n});
    const auto properties = call(b, "getNodeProperties", {n});
    emit({{"kind", "dynamic"}, {"events", *events.toJSON()}, {"data", *data.toJSON()}, {"same", properties.strictEquals(data.get("properties"))}});
    auto flowNode = JSValue::object(); flowNode.set("build", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { throw JSRangeError("failure"); }, {}, "build", 1));
    auto clean = builder(); const auto previous = clean.get("flow");
    const auto failure = attempt([&] { return call(clean, "flowChildNode", {flowNode}); });
    emit({{"kind", "abruptFlow"}, {"error", failure}, {"restored", clean.get("flow").strictEquals(previous)}, {"code", encode(clean.get("flow").get("code"))}});
}
void dataProtocol(unsigned mode) {
    auto self = JSValue::object(), n = JSValue::object(), cache = JSValue::object(), state = JSValue::object(), events = JSValue::array();
    state.set("events", events); state.set("data", mode & 4 ? JSValue{} : JSValue::object()); state.set("cache", cache); state.set("mode", mode);
    auto accessor = [&](JSValue target, const char* name, JSValue::NativeFunction fn, const Args& extra = {}) {
        JSPropertyDescriptor d; d.getter = JSValue::function(fn, extra.empty() ? Args{state} : extra, std::string("get ") + name, 0); d.configurable = true;
        target.defineProperty(name, d);
    };
    const auto stage = mode & 8 ? JSValue::object() : JSValue("fragment");
    if (stage.isObject()) {
        auto key = stage; key.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& captures) {
            call(captures[0].get("events"), "push", {"key:" + args[0].toString()}); return JSValue("fragment");
        }, {state}, "toPrimitive", 1));
    }
    accessor(self, "shaderStage", [](const JSValue&, const Args&, const Args& captures) {
        auto state = captures[0]; call(state.get("events"), "push", {"shaderStage"});
        if (state.get("mode").toNumber() >= 28) throw JSRangeError("stage"); return captures[1];
    }, {state, stage});
    accessor(n, "isGlobal", [](const JSValue&, const Args&, const Args& captures) {
        call(captures[0].get("events"), "push", {"isGlobal.get"});
        return JSValue::function([](const JSValue&, const Args&, const Args& c) {
            call(c[0].get("events"), "push", {"isGlobal.call"}); return JSValue(unsigned(c[0].get("mode").toNumber()) & 1 ? true : false);
        }, captures, "isGlobal", 1);
    });
    for (const auto* field : {"globalCache", "cache"}) accessor(self, field, [](const JSValue&, const Args&, const Args& captures) {
        call(captures[0].get("events"), "push", {captures[1]}); return captures[0].get("cache");
    }, {state, field});
    accessor(cache, "getData", [](const JSValue&, const Args&, const Args& captures) {
        call(captures[0].get("events"), "push", {"getData.get"});
        return JSValue::function([](const JSValue&, const Args&, const Args& c) {
            call(c[0].get("events"), "push", {"getData.call"}); return c[0].get("data");
        }, captures, "getData", 1);
    });
    accessor(cache, "setData", [](const JSValue&, const Args&, const Args& captures) {
        call(captures[0].get("events"), "push", {"setData.get"});
        return JSValue::function([](const JSValue&, const Args& items, const Args& c) -> JSValue {
            call(c[0].get("events"), "push", {"setData.call"}); auto state = c[0]; state.set("data", items[1]); return {};
        }, captures, "setData", 2);
    });
    accessor(self, "subBuildLayers", [](const JSValue&, const Args&, const Args& captures) {
        call(captures[0].get("events"), "push", {"subBuildLayers"}); return JSValue::array();
    });
    Args args{n}; if (mode & 2) args.push_back(stage); if (mode & 16) { if (args.size() == 1) args.push_back({}); args.push_back(cache); }
    JSValue result;
    const auto outcome = attempt([&] { result = NodeBuilder::prototype().get("getDataFromNode").call(self, args); return result; });
    emit({{"kind", "dataProtocol"}, {"mode", mode}, {"events", *events.toJSON()}, {"outcome", outcome},
        {"dataIdentity", !result.isUndefined() && result.strictEquals(state.get("data").get("fragment"))}});
}
void callOrder(unsigned mode) {
    auto b = builder(), n = node(), events = JSValue::array(); auto state = JSValue::object(); state.set("events", events); state.set("mode", mode);
    const auto accessor = [&](JSValue target, const char* field, const JSValue& fn) {
        JSPropertyDescriptor d; d.getter = fn; d.configurable = true; target.defineProperty(field, d);
    };
    auto fn = JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"outer.get"});
        if (c[0].get("mode").strictEquals(3)) throw JSRangeError("outer");
        return JSValue::function([](const JSValue&, const Args& args, const Args& captures) {
            call(captures[0].get("events"), "push", {"outer.call"}); return args.empty() ? JSValue{} : args[0];
        }, c, "outer", 0);
    }, {state}, "get outer", 0);
    JsonValue result;
    if (mode == 0 || mode == 3) {
        accessor(b, "setHashNode", fn);
        accessor(n, "getHash", JSValue::function([](const JSValue&, const Args&, const Args& c) {
            call(c[0].get("events"), "push", {"inner.get"});
            return JSValue::function([](const JSValue&, const Args&, const Args& captures) { call(captures[0].get("events"), "push", {"inner.call"}); return JSValue("hash"); }, c, "inner", 1);
        }, {state}, "get inner", 0));
        result = attempt([&] { return NodeBuilder::prototype().get("addNode").call(b, {n}); });
    } else if (mode == 1) {
        accessor(b, "getTypeFromLength", fn);
        accessor(b, "getTypeLength", JSValue::function([](const JSValue&, const Args&, const Args& c) {
            call(c[0].get("events"), "push", {"inner.get"});
            return JSValue::function([](const JSValue&, const Args&, const Args& captures) { call(captures[0].get("events"), "push", {"inner.call"}); return JSValue(3); }, c, "inner", 1);
        }, {state}, "get inner", 0));
        result = attempt([&] { return NodeBuilder::prototype().get("changeComponentType").call(b, {"vec3", "int"}); });
    } else {
        accessor(b, "getComponentType", fn);
        accessor(n, "getNodeType", JSValue::function([](const JSValue&, const Args&, const Args& c) {
            call(c[0].get("events"), "push", {"inner.get"});
            return JSValue::function([](const JSValue&, const Args&, const Args& captures) { call(captures[0].get("events"), "push", {"inner.call"}); return JSValue("float"); }, c, "inner", 1);
        }, {state}, "get inner", 0));
        result = attempt([&] { return NodeBuilder::prototype().get("getPromotedComponentType").call(b, {n}); });
    }
    emit({{"kind", "callOrder"}, {"mode", mode}, {"events", *events.toJSON()}, {"result", result}});
}
}
int main() {
    try {
        auto keeper = JSValue::object(); ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }));
        metadata(); typesAndFormat(); dataAndUsage(); declarationAndFlow(); nodeBuild(); dynamic();
        for (unsigned mode = 0; mode < 32; ++mode) dataProtocol(mode);
        for (unsigned mode = 0; mode < 4; ++mode) callOrder(mode);
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
