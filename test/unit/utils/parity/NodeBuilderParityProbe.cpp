#include "nodes/core/NodeBuilder.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeCache.h"
#include "nodes/core/NodeAttribute.h"
#include "nodes/core/NodeUniform.h"
#include "nodes/core/NodeVar.h"
#include "nodes/core/NodeVarying.h"
#include "nodes/core/NodeCode.h"
#include "nodes/core/StructType.h"
#include "renderers/common/BindGroup.h"
#include "nodes/code/FunctionNode.h"
#include "nodes/core/ParameterNode.h"
#include "renderers/common/nodes/NodeUniform.h"
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
void accessor(JSValue object, const char* name, const JSValue& getter, const JSValue& setter = {}) {
    JSPropertyDescriptor d; d.getter = getter; d.setter = setter; d.configurable = d.enumerable = true;
    object.defineProperty(name, d);
}
JSValue fields(std::initializer_list<std::pair<const char*, JSValue>> values) {
    auto result = JSValue::object(); for (const auto& [key, value] : values) result.set(key, value); return result;
}
JSValue array(const Args& values) {
    auto result = JSValue::array(); for (const auto& value : values) result.set(std::to_string(result.length()), value); return result;
}
void bindGroupMetadata() {
    const auto ctor = BindGroup::constructor();
    auto first = ctor.construct(), second = ctor.construct(), bindings = JSValue::array(), explicitGroup = ctor.construct({nullptr, bindings});
    auto normalized = snapshot(first); normalized["id"]["value"] = 0;
    emit({{"kind", "bindGroupMetadata"}, {"constructor", snapshot(ctor)}, {"prototype", snapshot(BindGroup::prototype())}, {"default", normalized},
        {"idGap", second.get("id").toNumber() - first.get("id").toNumber()}, {"separateArrays", !first.get("bindings").strictEquals(second.get("bindings"))},
        {"explicitName", encode(explicitGroup.get("name"))}, {"bindingsIdentity", explicitGroup.get("bindings").strictEquals(bindings)},
        {"instancePrototype", first.getPrototype().strictEquals(BindGroup::prototype())}, {"call", attempt([&] { return ctor.call(); })}});
}
void bindGroupShared(unsigned mode) {
    auto events = JSValue::array(), group = JSValue::object(), otherGroup = JSValue::object(), bindings = JSValue::object(), manager = JSValue::object(), otherManager = JSValue::object();
    auto first = JSValue::object(), second = JSValue::object(), third = JSValue::object(), sharedResult = JSValue::object(), renderer = JSValue::object(), self = JSValue::object();
    auto state = fields({{"events", events}, {"mode", mode}, {"group", group}, {"otherGroup", otherGroup}, {"bindings", bindings}, {"manager", manager}, {"otherManager", otherManager}, {"result", sharedResult}, {"reads", 0}});
    accessor(group, "shared", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"shared"}); return JSValue(unsigned(c[0].get("mode").toNumber()) & 1 ? JSValue("shared") : JSValue(false));
    }, {state}));
    unsigned i = 0;
    for (auto value : {first, second, third}) {
        const auto index = i++;
        accessor(value, "groupNode", JSValue::function([](const JSValue&, const Args&, const Args& c) {
            const auto index = unsigned(c[1].toNumber()), mode = unsigned(c[0].get("mode").toNumber());
            call(c[0].get("events"), "push", {"group" + JSValue(index).toString()});
            if (index == 2 && (mode & 16)) throw JSRangeError("third group");
            return index == 1 && (mode & 2) ? c[0].get("otherGroup") : c[0].get("group");
        }, {state, index}));
        accessor(bindings, JSValue(index).toString().c_str(), JSValue::function([](const JSValue&, const Args&, const Args& c) {
            call(c[0].get("events"), "push", {"binding" + c[1].toString()}); return c[2];
        }, {state, index, value}));
    }
    accessor(bindings, "length", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"length"}); return JSValue(3);
    }, {state}));
    for (const auto& value : {manager, otherManager}) accessor(value, "getSharedBindGroup", JSValue::function([](const JSValue& self, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"method.get"});
        return JSValue::function([](const JSValue& self, const Args& args, const Args& c) {
            call(c[0].get("events"), "push", {array({"method.call", self.strictEquals(c[1]), args[0], args[1].strictEquals(c[0].get("bindings"))})}); return c[0].get("result");
        }, {c[0], self}, "getSharedBindGroup", 2);
    }, {state}));
    accessor(renderer, "_bindings", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        auto s = c[0]; const auto reads = s.get("reads").toNumber(); s.set("reads", reads + 1);
        call(s.get("events"), "push", {"_bindings" + JSValue(reads).toString()});
        const auto mode = unsigned(s.get("mode").toNumber()); if (mode & 4) return JSValue{};
        return reads != 0 && (mode & 8) ? s.get("otherManager") : s.get("manager");
    }, {state}));
    accessor(self, "renderer", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"renderer"}); return c[1];
    }, {state, renderer}));
    JSValue result; const auto outcome = attempt([&] { result = NodeBuilder::prototype().get("_getBindGroup").call(self, {"frame", bindings}); return result; });
    emit({{"kind", "bindGroupShared"}, {"mode", mode}, {"events", *events.toJSON()}, {"outcome", outcome},
        {"shared", result.strictEquals(sharedResult)}, {"local", result.isObject() && result.getPrototype().strictEquals(BindGroup::prototype())},
        {"bindingsIdentity", result.isObject() && !result.strictEquals(sharedResult) && result.get("bindings").strictEquals(bindings)}});
}
void bindGroupBasic() {
    auto b = builder(), group = fields({{"shared", false}, {"order", 7}}), other = fields({{"shared", false}, {"order", -1}});
    auto a = fields({{"label", "a"}, {"groupNode", group}}), c = fields({{"label", "c"}, {"groupNode", group}}), z = fields({{"label", "z"}, {"groupNode", other}});
    call(call(b, "getBindGroupArray", {"frame", "fragment"}), "push", {a, a});
    call(call(b, "getBindGroupArray", {"frame", "vertex"}), "push", {c, a});
    call(call(b, "getBindGroupArray", {"frame", "compute"}), "push", {c});
    call(call(b, "getBindGroupArray", {"object", "vertex"}), "push", {z});
    const auto groups = call(b, "getBindings"); A rows;
    for (unsigned i = 0; i < groups.length(); ++i) {
        const auto value = groups.get(JSValue(i)); A labels;
        for (unsigned j = 0; j < value.get("bindings").length(); ++j) labels.push_back(encode(value.get("bindings").get(JSValue(j)).get("label")));
        rows.push_back(O{{"name", encode(value.get("name"))}, {"labels", labels}, {"prototype", value.getPrototype().strictEquals(BindGroup::prototype())}});
    }
    const auto cached = call(b, "getBindings").strictEquals(groups); call(b, "sortBindingGroups");
    emit({{"kind", "bindGroupBasic"}, {"groups", rows}, {"cached", cached}, {"sorted", A{encode(groups.get("0").get("name")), encode(groups.get("1").get("name"))}},
        {"indexes", *b.get("bindingsIndexes").toJSON()}});
}
void bindGroupsEnumeration(unsigned mode) {
    auto self = builder(), events = JSValue::array(), inherited = JSValue::object(), fragment = JSValue::object(inherited), vertex = JSValue::object(), bindings = self.get("bindings");
    auto values = [](const char* label) { return array({fields({{"label", label}})}); };
    fragment.set("2", values("two")); fragment.set("later", values("later")); inherited.set("inherited", values("inherited"));
    JSPropertyDescriptor hidden; hidden.value = values("hidden"); fragment.defineProperty("shadow", hidden); inherited.set("shadow", values("shadow"));
    auto state = fields({{"events", events}, {"mode", mode}, {"fragment", fragment}, {"inherited", inherited}, {"vertex", vertex}, {"count", 0}, {"self", self}});
    accessor(fragment, "1", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        auto state = c[0]; const auto mode = unsigned(state.get("mode").toNumber()); call(state.get("events"), "push", {"first.get"});
        auto fragment = state.get("fragment"), inherited = state.get("inherited"), vertex = state.get("vertex");
        if (mode & 1) fragment.erase("later");
        if (mode & 2) { fragment.set("added", array({fields({{"label", "added"}})})); inherited.set("addedPrototype", array({fields({{"label", "addedPrototype"}})})); }
        if (mode & 4) vertex.set("nextStage", array({fields({{"label", "nextStage"}})}));
        if (mode & 8) {
            auto replacement = fields({{"fragment", fields({{"replaced", array({fields({{"label", "replaced"}})})}})}});
            auto self = state.get("self"); self.set("bindings", replacement);
        }
        return array({fields({{"label", "one"}})});
    }, {state}));
    bindings.set("fragment", fragment); bindings.set("vertex", vertex);
    accessor(self, "_getBindGroup", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"factory.get"});
        return JSValue::function([](const JSValue& self, const Args& args, const Args& c) {
            call(c[0].get("events"), "push", {array({"factory.call", args[0], self.strictEquals(c[0].get("self"))})});
            auto labels = JSValue::array(); for (unsigned i = 0; i < args[1].length(); ++i) call(labels, "push", {args[1].get(JSValue(i)).get("label")});
            return fields({{"name", args[0]}, {"labels", labels}});
        }, c, "_getBindGroup", 2);
    }, {state}));
    JSValue result; const auto outcome = attempt([&] { result = NodeBuilder::prototype().get("getBindings").call(self); return result; });
    emit({{"kind", "bindGroupsEnumeration"}, {"mode", mode}, {"outcome", outcome}, {"events", *events.toJSON()},
        {"groups", result.isObject() ? *result.toJSON() : JsonValue(nullptr)}, {"cached", result.strictEquals(self.get("bindGroups"))}, {"bindingsReplaced", !self.get("bindings").strictEquals(bindings)}});
}
void bindGroupsFailure(unsigned mode) {
    auto self = builder(), events = JSValue::array(), state = fields({{"mode", mode}, {"events", events}, {"count", 0}});
    auto bindings = self.get("bindings"); auto fragment = fields({{"first", array({1, 1, 2})}, {"second", array({3})}}); bindings.set("fragment", fragment);
    const auto originalIncludes = JSValue::array().getPrototype().getOwnPropertyDescriptor("includes");
    if (mode < 3) {
        JSPropertyDescriptor d; d.configurable = d.writable = true;
        d.value = JSValue::function([](const JSValue&, const Args& args, const Args& c) {
            call(c[0].get("events"), "push", {array({"includes", args[0]})});
            return c[0].get("mode").strictEquals(0) ? JSValue(false) : c[0].get("mode").strictEquals(1) ? JSValue(0) : JSValue{};
        }, {state}, "includes", 1);
        JSValue::array().getPrototype().defineProperty("includes", d);
    }
    self.set("_getBindGroup", JSValue::function([](const JSValue&, const Args& args, const Args& c) {
        auto state = c[0]; const auto count = state.get("count").toNumber(); state.set("count", count + 1);
        call(state.get("events"), "push", {array({"factory", args[0], args[1]})});
        if (state.get("mode").strictEquals(3) && count == 1) throw JSRangeError("factory");
        return fields({{"name", args[0]}, {"values", args[1]}});
    }, {state}, "_getBindGroup", 2));
    if (mode == 4) accessor(self, "bindGroups", JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue(nullptr); }),
        JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0].get("events"), "push", {"bindGroups.set"}); throw JSRangeError("set"); }, {state}));
    if (mode >= 5 && mode <= 7) {
        auto special = JSValue::object(); special.set(mode == 5 ? "constructor" : mode == 6 ? "toString" : "__proto__", array({1})); bindings.set("fragment", special);
    }
    if (mode >= 8) self.set("bindGroups", mode == 8 ? JSValue{} : mode == 9 ? JSValue(false) : mode == 10 ? JSValue(0) : JSValue("cached"));
    JSValue result; const auto outcome = attempt([&] { result = NodeBuilder::prototype().get("getBindings").call(self); return result; });
    if (mode < 3) JSValue::array().getPrototype().defineProperty("includes", *originalIncludes);
    emit({{"kind", "bindGroupsFailure"}, {"mode", mode}, {"events", *events.toJSON()}, {"outcome", outcome},
        {"groups", result.isObject() ? *result.toJSON() : encode(result)}, {"cache", encode(self.get("bindGroups"))}});
}
void bindGroupsSort(unsigned mode) {
    auto self = builder(), events = JSValue::array(), groups = JSValue::array(), indexes = JSValue::object(), replacement = JSValue::object();
    const auto state = fields({{"mode", mode}, {"events", events}, {"self", self}, {"groups", groups}, {"indexes", indexes}, {"replacement", replacement}, {"conversions", 0}});
    for (unsigned i = 0; i < 8; ++i) {
        const auto label = "group" + JSValue(i).toString();
        auto group = JSValue::object(), binding = JSValue::object(), groupNode = JSValue::object(), order = JSValue::object(), entry = fields({{"group", -1}}), otherEntry = fields({{"group", -2}});
        auto local = fields({{"state", state}, {"label", label}, {"index", i}, {"order", order}});
        order.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& c) {
            const auto local = c[0], state = local.get("state"); call(state.get("events"), "push", {array({"convert", local.get("label"), args[0]})});
            auto mutableState = state; const auto conversions = state.get("conversions").toNumber(); mutableState.set("conversions", conversions + 1);
            if (state.get("mode").strictEquals(2) && conversions == 1) throw JSRangeError("convert");
            const auto index = unsigned(local.get("index").toNumber()); return JSValue((index * 5 + 3) % 6);
        }, {local}, "toPrimitive", 1));
        accessor(groupNode, "order", JSValue::function([](const JSValue&, const Args&, const Args& c) {
            const auto state = c[0].get("state"); call(state.get("events"), "push", {array({"order.get", c[0].get("label")})});
            if (state.get("mode").strictEquals(1)) throw JSRangeError("order"); return c[0].get("order");
        }, {local}));
        binding.set("groupNode", groupNode); group.set("bindings", array({binding})); group.set("testLabel", label);
        accessor(group, "name", JSValue::function([](const JSValue&, const Args&, const Args& c) {
            auto state = c[0].get("state"); call(state.get("events"), "push", {array({"name.get", c[0].get("label")})});
            if (state.get("mode").strictEquals(6)) { auto self = state.get("self"); self.set("bindingsIndexes", state.get("replacement")); }
            return c[0].get("label");
        }, {local}));
        if (mode == 5) accessor(entry, "group", JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue(-1); }),
            JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue {
                const auto state = c[0].get("state"); call(state.get("events"), "push", {array({"group.set", c[0].get("label"), args[0]})});
                if (args[0].strictEquals(1)) throw JSRangeError("index"); return {};
            }, {local}, "set group", 1));
        indexes.set(label, entry); replacement.set(label, otherEntry); groups.set(JSValue(i), group);
    }
    self.set("bindingsIndexes", indexes);
    accessor(self, "getBindings", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"getBindings.get"}); if (c[0].get("mode").strictEquals(4)) throw JSRangeError("getBindings");
        return JSValue::function([](const JSValue&, const Args&, const Args& c) { call(c[0].get("events"), "push", {"getBindings.call"}); return c[0].get("groups"); }, c, "getBindings", 0);
    }, {state}));
    const auto nativeSort = groups.get("sort");
    accessor(groups, "sort", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"sort.get"}); if (c[0].get("mode").strictEquals(3)) throw JSRangeError("sort");
        return JSValue::function([](const JSValue& receiver, const Args& args, const Args& c) {
            call(c[0].get("events"), "push", {array({"sort.call", receiver.strictEquals(c[0].get("groups")), args[0].get("length")})});
            if (c[0].get("mode").strictEquals(7)) return JSValue::array();
            return c[1].call(receiver, args);
        }, c, "sort", 1);
    }, {state, nativeSort}));
    const auto outcome = attempt([&] { return NodeBuilder::prototype().get("sortBindingGroups").call(self); });
    A names, originalIndex, replacedIndex;
    for (unsigned i = 0; i < 8; ++i) { const auto label = "group" + JSValue(i).toString(); originalIndex.push_back(encode(indexes.get(label).get("group"))); replacedIndex.push_back(encode(replacement.get(label).get("group"))); }
    for (unsigned i = 0; i < groups.length(); ++i) names.push_back(encode(groups.get(JSValue(i)).get("testLabel")));
    emit({{"kind", "bindGroupsSort"}, {"mode", mode}, {"outcome", outcome}, {"events", *events.toJSON()}, {"names", names}, {"indexes", originalIndex}, {"replacement", replacedIndex}});
}
void bindGroupsIterator(unsigned mode) {
    auto self = builder(), events = JSValue::array(), iterable = JSValue::object(), iterator = JSValue::object();
    auto state = fields({{"mode", mode}, {"events", events}, {"count", 0}});
    iterator.set("next", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        auto state = c[0], events = state.get("events"); events.set(JSValue(events.length()), "next");
        const auto mode = unsigned(state.get("mode").toNumber()); const auto count = state.get("count").toNumber(); state.set("count", count + 1);
        if (mode == 0) throw JSRangeError("next");
        if (mode == 4) return JSValue(1);
        auto result = fields({{"done", count != 0}});
        if (mode == 5) accessor(result, "value", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            auto events = c[0].get("events"); events.set(JSValue(events.length()), "value.get"); throw JSRangeError("value");
        }, {state}));
        else result.set("value", 1);
        return result;
    }, {state}, "next", 0));
    accessor(iterator, "return", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        auto events = c[0].get("events"); events.set(JSValue(events.length()), "return.get");
        if (c[0].get("mode").strictEquals(3)) throw JSTypeError("return getter");
        return JSValue::function([](const JSValue&, const Args&, const Args& c) {
            auto events = c[0].get("events"); events.set(JSValue(events.length()), "return.call"); return JSValue::object();
        }, c, "return", 0);
    }, {state}));
    iterable.set(JSValue::wellKnownSymbol("iterator"), JSValue::function([](const JSValue&, const Args&, const Args& c) { return c[0]; }, {iterator}));
    auto bindings = self.get("bindings"); bindings.set("fragment", fields({{"frame", iterable}}));
    self.set("_getBindGroup", JSValue::function([](const JSValue&, const Args& args, const Args&) { return fields({{"name", args[0]}, {"values", args[1]}}); }));
    auto prototype = JSValue::array().getPrototype(); const auto original = *prototype.getOwnPropertyDescriptor("includes");
    if (mode == 1 || mode == 2 || mode == 3) {
        JSPropertyDescriptor d; d.configurable = true;
        d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            auto events = c[0].get("events"); events.set(JSValue(events.length()), "includes.get");
            if (!c[0].get("mode").strictEquals(2)) throw JSRangeError("includes getter");
            return JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
                auto events = c[0].get("events"); events.set(JSValue(events.length()), "includes.call"); throw JSRangeError("includes call");
            }, c, "includes", 1);
        }, {state}); prototype.defineProperty("includes", d);
    }
    const auto outcome = attempt([&] { return NodeBuilder::prototype().get("getBindings").call(self); });
    if (mode == 1 || mode == 2 || mode == 3) prototype.defineProperty("includes", original);
    emit({{"kind", "bindGroupsIterator"}, {"mode", mode}, {"outcome", outcome}, {"events", *events.toJSON()}, {"cache", encode(self.get("bindGroups"))}});
}
void functionCacheBasic() {
    auto backend = JSValue::object(), otherBackend = JSValue::object(), shader = JSValue::object(), otherShader = JSValue::object(), calls = JSValue::array();
    auto first = builder(), second = builder();
    for (auto self : {first, second}) {
        auto r = self.get("renderer"); r.set("backend", backend);
        self.set("buildFunctionCode", JSValue::function([](const JSValue& self, const Args& args, const Args& c) {
            call(c[0], "push", {array({self.get("currentFunctionNode").getPrototype().strictEquals(FunctionNode::prototype()), args[0].strictEquals(c[1])})});
            return JSValue("float functionCode() { return 2.0; }");
        }, {calls, shader}, "buildFunctionCode", 1));
    }
    const auto a = call(first, "buildFunctionNode", {shader}), again = call(second, "buildFunctionNode", {shader}), other = call(second, "buildFunctionNode", {otherShader});
    auto r = second.get("renderer"); r.set("backend", otherBackend); const auto differentBackend = call(second, "buildFunctionNode", {shader});
    emit({{"kind", "functionCacheBasic"}, {"same", a.strictEquals(again)}, {"otherShader", !a.strictEquals(other)}, {"otherBackend", !a.strictEquals(differentBackend)},
        {"prototype", a.getPrototype().strictEquals(FunctionNode::prototype())}, {"code", encode(a.get("code"))}, {"calls", *calls.toJSON()}, {"restored", first.get("currentFunctionNode").isNull() && second.get("currentFunctionNode").isNull()}});
}
void functionCacheProtocol(unsigned mode) {
    auto self = JSValue::object(), events = JSValue::array(), backend = mode & 4 ? JSValue(7) : JSValue::object(), shader = mode & 8 ? JSValue(9) : JSValue::object(), previous = JSValue::object();
    auto state = fields({{"mode", mode}, {"events", events}, {"current", previous}, {"previous", previous}, {"self", self}, {"shader", shader}}), r = JSValue::object();
    accessor(self, "renderer", JSValue::function([](const JSValue&, const Args&, const Args& c) { call(c[0].get("events"), "push", {"renderer"}); return c[1]; }, {state, r}));
    accessor(r, "backend", JSValue::function([](const JSValue&, const Args&, const Args& c) { call(c[0].get("events"), "push", {"backend"}); return c[1]; }, {state, backend}));
    accessor(self, "currentFunctionNode", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"current.get"}); return c[0].get("current");
    }, {state}), JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue {
        auto state = c[0]; call(state.get("events"), "push", {array({"current.set", args[0].strictEquals(state.get("previous")) ? JSValue("previous") : JSValue("function")})}); state.set("current", args[0]); return {};
    }, {state}, "set currentFunctionNode", 1));
    accessor(self, "buildFunctionCode", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"build.get"}); const auto mode = unsigned(c[0].get("mode").toNumber()); if (mode & 1) throw JSRangeError("build getter");
        return JSValue::function([](const JSValue& self, const Args& args, const Args& c) {
            call(c[0].get("events"), "push", {array({"build.call", self.strictEquals(c[0].get("self")), args[0].strictEquals(c[0].get("shader")), c[0].get("current").getPrototype().strictEquals(FunctionNode::prototype())})});
            if (unsigned(c[0].get("mode").toNumber()) & 2) throw JSRangeError("build call"); return JSValue("compiled");
        }, c, "buildFunctionCode", 1);
    }, {state}));
    JSValue first, second;
    const auto outcome = attempt([&] { first = NodeBuilder::prototype().get("buildFunctionNode").call(self, {shader}); return first; });
    const auto partial = state.get("current");
    const auto repeated = attempt([&] { second = NodeBuilder::prototype().get("buildFunctionNode").call(self, {shader}); return second; });
    emit({{"kind", "functionCacheProtocol"}, {"mode", mode}, {"outcome", outcome}, {"repeated", repeated}, {"events", *events.toJSON()},
        {"cached", first.isObject() && first.strictEquals(second)}, {"partialPrevious", partial.strictEquals(previous)}, {"currentPrevious", state.get("current").strictEquals(previous)},
        {"partialChanged", !partial.strictEquals(state.get("current"))}, {"code", first.isObject() ? encode(first.get("code")) : JsonValue(nullptr)}});
}
void rendererNodeUniform(unsigned index) {
    const std::vector<std::string> names{"float", "int", "uint", "vec2", "ivec2", "uvec2", "vec3", "ivec3", "uvec3", "vec4", "ivec4", "uvec4", "color", "mat2", "mat3", "mat4", "bool", "bvec2", "texture", "unknown"};
    const auto type = names[index]; auto self = builder(), source = fields({{"name", "uniform"}, {"value", 7}, {"type", type}});
    JSValue result; const auto outcome = attempt([&] { result = call(self, "getNodeUniform", {source, type}); return result; });
    O row{{"kind", "rendererNodeUniform"}, {"index", index}, {"type", type}, {"outcome", outcome}};
    if (result.isObject()) {
        row["snapshot"] = snapshot(result); row["nodeIdentity"] = result.get("nodeUniform").strictEquals(source);
        row["firstValue"] = encode(call(result, "getValue")); source.set("value", 19); row["liveValue"] = encode(call(result, "getValue")); row["liveType"] = encode(call(result, "getType"));
        const auto className = index < 3 ? "NumberNodeUniform" : index < 6 ? "Vector2NodeUniform" : index < 9 ? "Vector3NodeUniform" : index < 12 ? "Vector4NodeUniform" : index == 12 ? "ColorNodeUniform" : index == 13 ? "Matrix2NodeUniform" : index == 14 ? "Matrix3NodeUniform" : "Matrix4NodeUniform";
        row["prototype"] = result.getPrototype().strictEquals(nativeNodeUniforms().get(className).get("prototype"));
    }
    emit(row);
}
void flowShaderProtocol(unsigned mode) {
    auto events = JSValue::array(), self = JSValue::object(), shader = JSValue::object(), layout = JSValue::object(), iterable = JSValue::object(), iterator = JSValue::object(), input = JSValue::object(), callNode = JSValue::object(), result = JSValue::object();
    auto state = fields({{"events", events}, {"mode", mode}, {"shader", shader}, {"layout", layout}, {"currentLayout", layout}, {"iterator", iterator}, {"input", input}, {"callNode", callNode}, {"result", result}, {"count", 0}, {"inputs", JSValue{}}, {"names", 0}});
    accessor(shader, "layout", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"layout.get"}); if (c[0].get("mode").strictEquals(16)) throw JSRangeError("layout"); return c[0].get("currentLayout");
    }, {state}), JSValue::function([](const JSValue&, const Args& args, const Args& c) {
        auto s = c[0]; call(s.get("events"), "push", {array({"layout.set", args[0].isNull() ? "null" : "original"})});
        if ((s.get("mode").strictEquals(6) && args[0].isNull()) || (s.get("mode").strictEquals(7) && !args[0].isNull())) throw JSRangeError("layout set");
        s.set("currentLayout", args[0]); return JSValue{};
    }, {state}));
    accessor(layout, "inputs", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"inputs.get"}); if (c[0].get("mode").strictEquals(17)) throw JSRangeError("inputs"); return c[1];
    }, {state, iterable}));
    accessor(layout, "type", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"type.get"}); if (c[0].get("mode").strictEquals(4)) throw JSRangeError("type"); return JSValue("vec3");
    }, {state}));
    JSPropertyDescriptor iteration; iteration.enumerable = iteration.configurable = true;
    iteration.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"iterator.get"}); if (c[0].get("mode").strictEquals(15)) throw JSRangeError("iterator");
        return JSValue::function([](const JSValue&, const Args&, const Args& c) { call(c[0].get("events"), "push", {"iterator.call"}); return c[0].get("iterator"); }, c, "[Symbol.iterator]", 0);
    }, {state}); iterable.defineProperty(JSValue::wellKnownSymbol("iterator"), iteration);
    iterator.set("next", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        auto s = c[0]; call(s.get("events"), "push", {"next"}); if (s.get("mode").strictEquals(10)) throw JSRangeError("next");
        const auto count = s.get("count").toNumber(); s.set("count", count + 1); return fields({{"done", count != 0}, {"value", s.get("input")}});
    }, {state}, "next", 0));
    accessor(iterator, "return", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"return.get"}); if (c[0].get("mode").strictEquals(11)) throw JSTypeError("return");
        return JSValue::function([](const JSValue&, const Args&, const Args& c) { call(c[0].get("events"), "push", {"return.call"}); return JSValue::object(); }, c, "return", 0);
    }, {state}));
    accessor(input, "name", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        auto s = c[0]; const auto count = s.get("names").toNumber(); s.set("names", count + 1); call(s.get("events"), "push", {array({"name.get", count})});
        if (s.get("mode").strictEquals(8) || s.get("mode").strictEquals(11) || (s.get("mode").strictEquals(12) && count == 1)) throw JSRangeError("name");
        return s.get("mode").strictEquals(13) ? JSValue{} : s.get("mode").strictEquals(14) ? JSValue(nullptr) : JSValue("weight");
    }, {state}));
    accessor(input, "type", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"input.type"}); if (c[0].get("mode").strictEquals(9)) throw JSRangeError("input type"); return JSValue("float");
    }, {state}));
    accessor(shader, "call", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"call.get"}); if (c[0].get("mode").strictEquals(1)) throw JSRangeError("call getter");
        if (c[0].get("mode").strictEquals(18)) return JSValue(0);
        return JSValue::function([](const JSValue& receiver, const Args& args, const Args& c) {
            auto s = c[0]; call(s.get("events"), "push", {array({"call.call", receiver.strictEquals(s.get("shader")), s.get("currentLayout").isNull()})});
            s.set("inputs", args[0]); if (s.get("mode").strictEquals(2)) throw JSRangeError("call"); return s.get("callNode");
        }, c, "call", 1);
    }, {state}));
    accessor(self, "flowStagesNode", JSValue::function([](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("events"), "push", {"flow.get"}); if (c[0].get("mode").strictEquals(3)) throw JSRangeError("flow getter");
        if (c[0].get("mode").strictEquals(19)) return JSValue(0);
        return JSValue::function([](const JSValue&, const Args& args, const Args& c) {
            call(c[0].get("events"), "push", {array({"flow.call", args[0].strictEquals(c[0].get("callNode")), args[1]})});
            if (c[0].get("mode").strictEquals(5)) throw JSRangeError("flow"); return c[0].get("result");
        }, c, "flowStagesNode", 2);
    }, {state}));
    JSValue value; const auto outcome = attempt([&] { value = NodeBuilder::prototype().get("flowShaderNode").call(self, {shader}); return value; });
    A inputs;
    if (state.get("inputs").isObject()) for (const auto& key : state.get("inputs").ownPropertyNames()) {
        const auto n = state.get("inputs").get(key);
        inputs.push_back(O{{"key", key}, {"name", encode(n.get("name"))}, {"nodeType", encode(n.get("nodeType"))}, {"parameter", n.getPrototype().strictEquals(ParameterNode::prototype())}, {"global", encode(n.get("global"))}, {"varying", encode(n.get("varying"))}, {"placeholder", encode(n.get("placeholderNode"))}});
    }
    emit({{"kind", "flowShaderProtocol"}, {"mode", mode}, {"outcome", outcome}, {"events", *events.toJSON()}, {"inputs", inputs}, {"restored", state.get("currentLayout").strictEquals(layout)}, {"nullLayout", state.get("currentLayout").isNull()}, {"resultIdentity", value.strictEquals(result)}});
}
void flowShaderIterator(unsigned mode) {
    auto self = JSValue::object(), shader = fields({{"layout", fields({{"inputs", JSValue::array()}, {"type", "float"}})}}), events = JSValue::array();
    auto state = fields({{"mode", mode}, {"events", events}, {"rows", JSValue::array()}});
    shader.set("call", JSValue::function([](const JSValue&, const Args& args, const Args& c) {
        auto state = c[0], borrowed = fields({{"2", 2}, {"1", 1}, {"tail", 4}}); auto inputs = args[0];
        auto local = fields({{"state", state}, {"borrowed", borrowed}});
        borrowed.set(JSValue::symbol("skip"), 99);
        accessor(borrowed, "first", JSValue::function([](const JSValue&, const Args&, const Args& c) {
            call(c[0].get("state").get("events"), "push", {"first.get"}); auto borrowed = c[0].get("borrowed"); borrowed.erase("tail"); borrowed.set("added", 5); return JSValue(3);
        }, {local}));
        // Reinsert tail after the getter so deletion is visible during Object.values.
        borrowed.erase("tail"); borrowed.set("tail", 4);
        auto intrinsic = JSValue::intrinsicObjectConstructor(); const auto original = *intrinsic.getOwnPropertyDescriptor("values");
        const auto mode = unsigned(state.get("mode").toNumber());
        if (mode != 0) {
            JSPropertyDescriptor d; d.configurable = true;
            d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) {
                call(c[0].get("state").get("events"), "push", {"values.get"}); const auto mode = unsigned(c[0].get("state").get("mode").toNumber());
                if (mode == 1) throw JSRangeError("values getter"); if (mode == 5) return JSValue(0);
                return JSValue::function([](const JSValue& receiver, const Args& args, const Args& c) {
                    auto state = c[0].get("state"); call(state.get("events"), "push", {array({"values.call", receiver.strictEquals(JSValue::intrinsicObjectConstructor()), args[0].strictEquals(c[0].get("borrowed"))})});
                    if (state.get("mode").strictEquals(2)) throw JSRangeError("values");
                    if (state.get("mode").strictEquals(6)) return JSValue(nullptr);
                    auto values = JSValue::object();
                    for (unsigned i = 0; i < 4; ++i) accessor(values, JSValue(i).toString().c_str(), JSValue::function([](const JSValue&, const Args&, const Args& c) {
                        call(c[0].get("events"), "push", {array({"value.get", c[1]})}); if (c[0].get("mode").strictEquals(3) && c[1].strictEquals(1)) throw JSRangeError("value"); return JSValue(9 - c[1].toNumber());
                    }, {state, i}));
                    accessor(values, "length", JSValue::function([](const JSValue&, const Args&, const Args& c) {
                        call(c[0].get("events"), "push", {"length.get"}); if (c[0].get("mode").strictEquals(7)) throw JSRangeError("length"); return JSValue(1);
                    }, {state})); return values;
                }, c, "values", 1);
            }, {local}); intrinsic.defineProperty("values", d);
        }
        const auto method = inputs.get(JSValue::wellKnownSymbol("iterator"));
        JSValue iterator;
        const auto outcome = attempt([&] { iterator = method.call(borrowed); return iterator; });
        intrinsic.defineProperty("values", original);
        // The iterator's next closure keeps the captured values after values is restored.
        A rows;
        if (iterator.isObject()) {
            for (unsigned i = 0; i < 4; ++i) {
                JSValue row; const auto next = attempt([&] { row = iterator.get("next").call(i == 1 ? JSValue(nullptr) : iterator); return row; });
                rows.push_back(O{{"outcome", next}, {"row", row.isObject() ? *row.toJSON() : encode(row)}});
            }
        }
        auto output = state; output.set("protocol", fields({{"methodName", method.get("name")}, {"methodLength", method.get("length")}, {"symbolEnumerable", inputs.getOwnPropertyDescriptor(JSValue::wellKnownSymbol("iterator"))->enumerable.value_or(false)}, {"iteratorKeys", iterator.isObject() ? array({iterator.ownPropertyNames().empty() ? "" : iterator.ownPropertyNames()[0]}) : JSValue::array()}, {"iterable", iterator.isObject() && !iterator.get(JSValue::wellKnownSymbol("iterator")).isUndefined()}}));
        output.set("outcome", JSValue::fromJSON(outcome)); output.set("rows", JSValue::fromJSON(rows)); return JSValue::object();
    }, {state}, "call", 1));
    self.set("flowStagesNode", JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue("flow"); }, {}, "flowStagesNode", 2));
    const auto flow = attempt([&] { return NodeBuilder::prototype().get("flowShaderNode").call(self, {shader}); });
    emit({{"kind", "flowShaderIterator"}, {"mode", mode}, {"flow", flow}, {"outcome", *state.get("outcome").toJSON()}, {"protocol", *state.get("protocol").toJSON()}, {"rows", *state.get("rows").toJSON()}, {"events", *events.toJSON()}});
}
}
int main() {
    try {
        auto keeper = JSValue::object(); ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }));
        metadata(); typesAndFormat(); dataAndUsage(); declarationAndFlow(); nodeBuild(); dynamic();
        for (unsigned mode = 0; mode < 32; ++mode) dataProtocol(mode);
        for (unsigned mode = 0; mode < 4; ++mode) callOrder(mode);
        bindGroupMetadata(); bindGroupBasic();
        for (unsigned mode = 0; mode < 32; ++mode) bindGroupShared(mode);
        for (unsigned mode = 0; mode < 16; ++mode) bindGroupsEnumeration(mode);
        for (unsigned mode = 0; mode < 12; ++mode) bindGroupsFailure(mode);
        for (unsigned mode = 0; mode < 8; ++mode) bindGroupsSort(mode);
        for (unsigned mode = 0; mode < 6; ++mode) bindGroupsIterator(mode);
        functionCacheBasic();
        for (unsigned mode = 0; mode < 16; ++mode) functionCacheProtocol(mode);
        for (unsigned index = 0; index < 20; ++index) rendererNodeUniform(index);
        for (unsigned mode = 0; mode < 20; ++mode) flowShaderProtocol(mode);
        for (unsigned mode = 0; mode < 8; ++mode) flowShaderIterator(mode);
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
