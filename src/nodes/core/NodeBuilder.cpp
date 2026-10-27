#include "nodes/core/NodeBuilder.h"
#include "nodes/core/VarNode.h"
#include "nodes/functions/material/getAlphaHashThreshold.h"
#include "nodes/core/NodeBuilderAsync.h"
#include "nodes/core/constants.h"
#include "nodes/core/NodeCache.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/core/NodeAttribute.h"
#include "nodes/core/NodeUniform.h"
#include "nodes/core/NodeVar.h"
#include "nodes/core/NodeVarying.h"
#include "nodes/core/NodeCode.h"
#include "nodes/core/StructType.h"
#include "nodes/core/ParameterNode.h"
#include "nodes/code/FunctionNode.h"
#include "renderers/common/BindGroup.h"
#include "renderers/common/nodes/NodeUniform.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSSet.h"
#include "core/JSString.h"
#include "core/JSWeakMap.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "core/RenderTarget.h"
#include "renderers/common/CubeRenderTarget.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "../../constants.h"
#include "utils.h"
#include <algorithm>
#include <cmath>
#include <regex>
#include <set>

JSValue nativeStackNodeConstructor();
JSValue nativeGetCurrentStack();
void nativeSetCurrentStack(const JSValue& value);

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i, JSValue fallback = {}) {
    return i < args.size() && !args[i].isUndefined() ? args[i] : fallback;
}
JSValue raw(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
struct Invocation {
    JSValue receiver, method;
    JSValue operator()(const Args& args = {}) const { return method.call(receiver, args); }
};
Invocation invoke(const JSValue& receiver, const char* name) { return {receiver, receiver.get(name)}; }
void put(JSValue target, const char* key, const JSValue& value) { target.set(key, value); }
void put(JSValue target, const JSValue& key, const JSValue& value) { target.set(key, value); }
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive();
    const auto b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    return a.toNumber() + b.toNumber();
}
JSValue object(std::initializer_list<std::pair<const char*, JSValue>> fields = {}) {
    auto result = JSValue::object();
    for (const auto& [key, value] : fields) result.set(key, value);
    return result;
}
JSValue stages(bool arrays, bool index = false) {
    auto result = JSValue::object();
    for (const auto* name : {"vertex", "fragment", "compute"})
        result.set(name, arrays ? JSValue::array() : JSValue::object());
    if (index) result.set("index", 0);
    return result;
}
void spreadInto(JSValue result, JSValue source) {
        if (source.isUndefined() || source.isNull()) return;
        if (!source.isObject()) {
            if (source.type() == JSValue::Type::String) source = createNativeStringObject(source);
            else return;
        }
        for (const auto& key : source.ownPropertyKeys()) {
            const auto descriptor = source.getOwnPropertyDescriptor(key);
            if (descriptor && descriptor->enumerable.value_or(false)) {
                JSPropertyDescriptor copy;
                copy.value = source.get(key);
                copy.writable = copy.enumerable = copy.configurable = true;
                result.defineProperty(key, copy);
            }
        }
}
JSValue spread(const Args& sources) {
    auto result = JSValue::object();
    for (auto source : sources) spreadInto(result, source);
    return result;
}
std::vector<std::string> forIn(JSValue value) {
    std::vector<std::string> result;
    std::set<std::string> seen;
    while (!value.isNull() && !value.isUndefined()) {
        if (!value.isObject()) value = JSValue::intrinsicObjectConstructor().call({}, {value});
        for (const auto& key : value.ownPropertyNames()) {
            if (seen.contains(key)) continue;
            const auto d = value.getOwnPropertyDescriptor(key);
            if (!d) continue;
            seen.insert(key);
            if (d->enumerable.value_or(false)) result.push_back(key);
        }
        value = value.getPrototype();
    }
    return result;
}
template<class F> void forInSnapshot(const JSValue& value, F body) {
    // The pinned runtime snapshots initial own/prototype candidates, then
    // suppresses keys deleted before their turn. New keys await the next loop.
    const auto candidates = forIn(value);
    for (const auto& key : candidates) if (value.has(key)) body(JSValue(key));
}
template<class F> void each(const JSValue& values, F body) {
    auto iterator = getJSIterator(values);
    while (const auto value = stepJSIterator(iterator)) {
        try { body(*value); }
        catch (...) { closeJSIterator(iterator, std::current_exception()); }
    }
}
bool checkWriteUsage(const JSValue& data) {
    if (data.get("writeUsageCount").toNumber() > 0) return true;
    if (!data.get("subBuildsCache").isUndefined()) {
        for (const auto& key : forIn(data.get("subBuildsCache")))
            if (checkWriteUsage(data.get("subBuildsCache").get(key))) return true;
    }
    return false;
}
JSValue initialize(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    const auto nodeObject = raw(args, 0);
    self.set("object", nodeObject);
    auto material = nodeObject.toBoolean() ? nodeObject.get("material") : nodeObject;
    self.set("material", material.toBoolean() ? material : JSValue(nullptr));
    auto geometry = nodeObject.toBoolean() ? nodeObject.get("geometry") : nodeObject;
    self.set("geometry", geometry.toBoolean() ? geometry : JSValue(nullptr));
    self.set("compute", nullptr);
    self.set("renderer", raw(args, 1)); self.set("parser", raw(args, 2));
    self.set("scene", nullptr); self.set("camera", nullptr);
    self.set("nodes", createNativeSet()); self.set("sequentialNodes", createNativeSet());
    self.set("updateNodes", JSValue::array()); self.set("updateBeforeNodes", JSValue::array());
    self.set("updateAfterNodes", JSValue::array()); self.set("hashNodes", object());
    for (const auto* key : {"observer", "lightsNode", "environmentNode", "fogNode", "clippingContext"}) self.set(key, nullptr);
    self.set("hardwareClipping", false);
    self.set("vertexShader", nullptr); self.set("fragmentShader", nullptr); self.set("computeShader", nullptr);
    self.set("flowNodes", stages(true));
    self.set("flowCode", object({{"vertex", ""}, {"fragment", ""}, {"compute", ""}}));
    self.set("uniforms", stages(true, true)); self.set("structs", stages(true, true));
    self.set("types", stages(true, true)); self.set("bindings", stages(false));
    self.set("bindingsIndexes", object()); self.set("bindGroups", nullptr);
    self.set("attributes", JSValue::array()); self.set("bufferAttributes", JSValue::array());
    self.set("varyings", JSValue::array()); self.set("codes", object()); self.set("vars", object());
    self.set("declarations", object()); self.set("flow", object({{"code", ""}}));
    self.set("chaining", JSValue::array()); self.set("stack", nativeStackNodeConstructor().construct());
    self.set("stacks", JSValue::array()); self.set("tab", "\t"); self.set("currentFunctionNode", nullptr);
    self.set("context", object({{"material", self.get("material")}}));
    self.set("cache", nativeNodeCacheConstructor().construct()); self.set("globalCache", self.get("cache"));
    self.set("flowsData", createNativeWeakMap()); self.set("shaderStage", nullptr); self.set("buildStage", nullptr);
    self.set("subBuildLayers", JSValue::array()); self.set("activeStacks", JSValue::array());
    self.set("subBuildFn", nullptr); self.set("fnCall", nullptr); self.set("flowBlock", nullptr);
    auto state = captures[0];
    const auto id = state.get("id").toNumber(); state.set("id", id + 1);
    JSPropertyDescriptor descriptor; descriptor.value = id;
    self.defineProperty("id", descriptor);
    return {};
}
JSValue getDataFromNode(const JSValue& self, const Args& args) {
    const auto node = raw(args, 0);
    const auto stage = args.size() > 1 && !args[1].isUndefined() ? args[1] : self.get("shaderStage");
    auto cache = arg(args, 2, nullptr);
    if (cache.isNull()) cache = invoke(node, "isGlobal")({self}).toBoolean() ? self.get("globalCache") : self.get("cache");
    auto nodeData = invoke(cache, "getData")({node});
    if (nodeData.isUndefined()) { nodeData = object(); invoke(cache, "setData")({node, nodeData}); }
    if (nodeData.get(stage).isUndefined()) put(nodeData, stage, object());
    auto data = nodeData.get(stage);
    if (self.get("subBuildLayers").get("length").toNumber() == 0) return data;
    const auto any = nodeData.get("any");
    const auto subBuilds = any.toBoolean() ? nodeData.get("any").get("subBuilds") : JSValue(nullptr);
    const auto subBuild = invoke(self, "getClosestSubBuild")({subBuilds});
    if (subBuild.toBoolean()) {
        if (data.get("subBuildsCache").isUndefined()) put(data, "subBuildsCache", object());
        auto cached = data.get("subBuildsCache").get(subBuild);
        if (!cached.toBoolean()) { cached = object(); put(data.get("subBuildsCache"), subBuild, cached); }
        data = cached; put(data, "subBuilds", subBuilds);
    }
    return data;
}
JSValue closestSubBuild(const JSValue& self, JSValue data) {
    JSValue subBuilds;
    if (data.toBoolean() && data.get("isNode").toBoolean()) {
        if (data.get("isShaderCallNodeInternal").toBoolean()) subBuilds = data.get("shaderNode").get("subBuilds");
        else if (data.get("isStackNode").toBoolean()) { subBuilds = JSValue::array(); subBuilds.set("0", data.get("subBuild")); }
        else subBuilds = invoke(self, "getDataFromNode")({data, "any"}).get("subBuilds");
    } else if (isNativeSet(data)) {
        subBuilds = JSValue::array(); each(data, [&](const JSValue& value) { invoke(subBuilds, "push")({value}); });
    } else subBuilds = data;
    if (!subBuilds.toBoolean()) return nullptr;
    const auto layers = self.get("subBuildLayers");
    for (double i = subBuilds.get("length").toNumber() - 1; i >= 0; --i) {
        const auto subBuild = subBuilds.get(JSValue(i));
        if (invoke(layers, "includes")({subBuild}).toBoolean()) return subBuild;
    }
    return nullptr;
}
JSValue getTypeFromLength(const JSValue& self, const Args& args) {
    const auto length = raw(args, 0), component = arg(args, 1, "float");
    if (length.strictEquals(1)) return component;
    auto base = invoke(nativeNodeUtils(), "getTypeFromLength")({length});
    const auto prefix = component.strictEquals("float") ? JSValue("") : component.get("0");
    if (std::regex_search(component.toString(), std::regex("mat2")))
        base = invoke(base, "replace")({"vec", "mat"});
    return add(prefix, base);
}
std::string toFloat(const JSValue& value) {
    auto text = value.toString();
    if (text.find('e') != std::string::npos) {
        text.erase(std::remove(text.begin(), text.end(), '+'), text.end()); return text;
    }
    const auto numeric = value.toNumber();
    return JSValue(numeric).toString() + (JSValue(std::fmod(numeric, 1)).toBoolean() ? "" : ".0");
}
JSValue generateConst(const JSValue& self, const Args& args) {
    const auto type = raw(args, 0); auto value = arg(args, 1, nullptr);
    const auto t = type.toString();
    if (value.isNull()) {
        if (t == "float" || t == "int" || t == "uint") value = 0;
        else if (t == "bool") value = false;
        else if (t == "color") value = nativeColor(std::make_shared<Color>());
        else if (t == "vec2" || t == "uvec2" || t == "ivec2" || t == "bvec2") value = nativeVector(std::make_shared<Vector2>());
        else if (t == "vec3" || t == "uvec3" || t == "ivec3" || t == "bvec3") value = nativeVector(std::make_shared<Vector3>());
        else if (t == "vec4" || t == "uvec4" || t == "ivec4" || t == "bvec4") value = nativeVector(std::make_shared<Vector4>());
    }
    if (t == "float") return toFloat(value);
    if (t == "int") return JSValue(std::trunc(value.toNumber())).toString();
    if (t == "uint") return value.toNumber() >= 0 ? JSValue(std::trunc(value.toNumber())).toString() + "u" : "0u";
    if (t == "bool") return value.toBoolean() ? "true" : "false";
    if (t == "color") {
        const auto nativeType = invoke(self, "getType")({"vec3"}).toString();
        const auto r = toFloat(value.get("r")), g = toFloat(value.get("g")), b = toFloat(value.get("b"));
        return nativeType + "( " + r + ", " + g + ", " + b + " )";
    }
    const auto length = invoke(self, "getTypeLength")({type}).toNumber();
    const auto component = invoke(self, "getComponentType")({type});
    auto components = JSValue::array();
    if (length == 2 || length == 3 || (length == 4 && t != "mat2")) {
        for (const auto* field : {"x", "y", "z", "w"}) {
            if (components.length() >= length) break;
            invoke(components, "push")({invoke(self, "generateConst")({component, value.get(field)})});
        }
    } else if (length >= 4 && value.toBoolean() &&
        (value.get("isMatrix2").toBoolean() || value.get("isMatrix3").toBoolean() || value.get("isMatrix4").toBoolean())) {
        const auto callback = JSValue::function([](const JSValue&, const Args& items, const Args& captures) {
            return invoke(captures[0], "generateConst")({captures[1], raw(items, 0)});
        }, {self, component}, "generateConst", 1);
        components = invoke(value.get("elements"), "map")({callback});
    } else if (length > 4) return invoke(self, "getType")({type}).toString() + "()";
    else throw std::runtime_error("THREE.NodeBuilder: Type '" + t + "' not found in generate constant attempt.");
    return invoke(self, "getType")({type}).toString() + "( " + invoke(components, "join")({", "}).toString() + " )";
}
JSValue format(const JSValue& self, const Args& args) {
    auto snippet = raw(args, 0);
    auto from = invoke(self, "getVectorType")({raw(args, 1)});
    auto to = invoke(self, "getVectorType")({raw(args, 2)});
    if (from.strictEquals(to) || to.isNull() || invoke(self, "isReference")({to}).toBoolean()) return snippet;
    const auto fromLength = invoke(self, "getTypeLength")({from}).toNumber();
    const auto toLength = invoke(self, "getTypeLength")({to}).toNumber();
    if (fromLength == 16 && toLength == 9) return invoke(self, "getType")({to}).toString() + "( " + snippet.toString() + "[ 0 ].xyz, " + snippet.toString() + "[ 1 ].xyz, " + snippet.toString() + "[ 2 ].xyz )";
    if (fromLength == 9 && toLength == 4) return invoke(self, "getType")({to}).toString() + "( " + snippet.toString() + "[ 0 ].xy, " + snippet.toString() + "[ 1 ].xy )";
    if (fromLength > 4 || toLength > 4 || toLength == 0) return snippet;
    if (fromLength == toLength) return invoke(self, "getType")({to}).toString() + "( " + snippet.toString() + " )";
    if (fromLength > toLength) {
        snippet = to.strictEquals("bool") ? "all( " + snippet.toString() + " )" : snippet.toString() + "." + std::string("xyz").substr(0, std::size_t(toLength));
        const auto component = invoke(self, "getComponentType")({from});
        return invoke(self, "format")({snippet, invoke(self, "getTypeFromLength")({toLength, component}), to});
    }
    if ((toLength == 4 && fromLength > 1) || fromLength == 2) {
        const auto component = invoke(self, "getComponentType")({to});
        const auto vector = invoke(self, "getTypeFromLength")({toLength == 4 ? 3 : 2, component});
        const auto nativeType = invoke(self, "getType")({to}).toString();
        const auto inner = invoke(self, "format")({snippet, from, vector}).toString();
        const auto constant = invoke(self, "generateConst")({component, component.strictEquals("bool") ? JSValue(toLength == 4) : JSValue(toLength == 4 ? 1 : 0)}).toString();
        return nativeType + "( " + inner + ", " + constant + " )";
    }
    if (fromLength == 1 && toLength > 1 && !from.strictEquals(invoke(self, "getComponentType")({to}))) {
        const auto component = invoke(self, "getComponentType")({to});
        snippet = invoke(self, "getType")({component}).toString() + "( " + snippet.toString() + " )";
    }
    return invoke(self, "getType")({to}).toString() + "( " + snippet.toString() + " )";
}

JSValue dispatch(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    const auto& name = captures[0].string();
    const auto a = raw(args, 0);
    if (name == "isFlatShading") return self.get("material").get("flatShading").strictEquals(true) || invoke(self.get("geometry"), "hasAttribute")({"normal"}).strictEquals(false);
    if (name == "isOpaque") {
        const auto material = self.get("material");
        return material.get("transparent").strictEquals(false) && material.get("blending").strictEquals(NormalBlending) && material.get("alphaToCoverage").strictEquals(false);
    }
    if (name == "createRenderTarget") return nativeRenderTargetConstructor().construct({a, raw(args, 1), raw(args, 2)});
    if (name == "createCubeRenderTarget") return nativeCubeRenderTargetConstructor().construct({a, raw(args, 1)});
    if (name == "getTypeFromAttribute") {
        auto dataAttribute = a;
        if (a.get("isInterleavedBufferAttribute").toBoolean()) dataAttribute = a.get("data");
        const auto array = dataAttribute.get("array");
        const auto itemSize = a.get("itemSize");
        const auto normalized = a.get("normalized");
        JSValue arrayType;
        if (!instanceOfFloat16BufferAttribute(a) && !normalized.strictEquals(true)) arrayType = invoke(self, "getTypeFromArray")({array});
        return invoke(self, "getTypeFromLength")({itemSize, arrayType});
    }
    if (name == "includes") return invoke(self.get("nodes"), "has")({a});
    if (name == "getOutputType") {
        const auto index = arg(args, 0, 0);
        const auto target = invoke(self.get("renderer"), "getRenderTarget")();
        if (!target.isNull() && !target.get("textures").get(index).isUndefined())
            return invoke(nativeNodeUtils(), "getTextureType")({target.get("textures").get(index)});
        return "vec4";
    }
    if (name == "getOutputStructName") return {};
    if (name == "_getBindGroup") {
        const auto bindings = raw(args, 1);
        const auto groupNode = bindings.get("0").get("groupNode");
        auto shared = groupNode.get("shared");
        if (shared.toBoolean()) {
            for (double i = 1; i < bindings.get("length").toNumber(); ++i)
                if (!groupNode.strictEquals(bindings.get(JSValue(i)).get("groupNode"))) shared = false;
        }
        if (shared.toBoolean() && !self.get("renderer").get("_bindings").isUndefined())
            return invoke(self.get("renderer").get("_bindings"), "getSharedBindGroup")({a, bindings});
        return nativeBindGroupConstructor().construct({a, bindings});
    }
    if (name == "getBindGroupArray") {
        const auto bindings = self.get("bindings").get(raw(args, 1));
        auto group = bindings.get(a);
        if (group.isUndefined()) {
            if (self.get("bindingsIndexes").get(a).isUndefined()) {
                const auto length = self.get("bindingsIndexes").keys().size();
                put(self.get("bindingsIndexes"), a, object({{"binding", 0}, {"group", length}}));
            }
            group = JSValue::array(); put(bindings, a, group);
        }
        return group;
    }
    if (name == "getBindings") {
        auto bindingsGroups = self.get("bindGroups");
        if (bindingsGroups.isNull()) {
            auto groups = object();
            const auto bindings = self.get("bindings");
            for (const auto* stage : {"fragment", "vertex", "compute"}) {
                forInSnapshot(bindings.get(stage), [&](const JSValue& groupName) {
                    const auto uniforms = bindings.get(stage).get(groupName);
                    auto groupUniforms = groups.get(groupName);
                    if (!groupUniforms.toBoolean()) { groupUniforms = JSValue::array(); groups.set(groupName, groupUniforms); }
                    each(uniforms, [&](const JSValue& uniform) {
                        if (invoke(groupUniforms, "includes")({uniform}).strictEquals(false))
                            invoke(groupUniforms, "push")({uniform});
                    });
                });
            }
            bindingsGroups = JSValue::array();
            forInSnapshot(groups, [&](const JSValue& groupName) {
                const auto group = groups.get(groupName);
                const auto bindingsGroup = invoke(self, "_getBindGroup")({groupName, group});
                invoke(bindingsGroups, "push")({bindingsGroup});
            });
            self.set("bindGroups", bindingsGroups);
        }
        return bindingsGroups;
    }
    if (name == "sortBindingGroups") {
        const auto groups = invoke(self, "getBindings")();
        const auto comparator = JSValue::function([](const JSValue&, const Args& values, const Args&) {
            // Get both operands before ToNumeric; either getter may mutate the
            // other group, and left conversion must precede right conversion.
            const auto left = raw(values, 0).get("bindings").get("0").get("groupNode").get("order");
            const auto right = raw(values, 1).get("bindings").get("0").get("groupNode").get("order");
            const auto leftNumber = left.toNumber(); const auto rightNumber = right.toNumber();
            return JSValue(leftNumber - rightNumber);
        }, {}, "", 2);
        invoke(groups, "sort")({comparator});
        for (double i = 0; i < groups.get("length").toNumber(); ++i) {
            const auto group = groups.get(JSValue(i));
            // Evaluate the base of the assignment before the computed key.
            auto indexes = self.get("bindingsIndexes");
            auto index = indexes.get(group.get("name")); index.set("group", i);
        }
        return {};
    }
    if (name == "setHashNode") { put(self.get("hashNodes"), raw(args, 1), a); return {}; }
    if (name == "addNode") {
        if (invoke(self.get("nodes"), "has")({a}).strictEquals(false)) {
            invoke(self.get("nodes"), "add")({a});
            invoke(self, "setHashNode")({a, invoke(a, "getHash")({self})});
        }
        return {};
    }
    if (name == "addSequentialNode") {
        const auto before = a.get("updateBeforeType"), after = a.get("updateAfterType");
        if (!before.strictEquals("none") || !after.strictEquals("none")) invoke(self.get("sequentialNodes"), "add")({a});
        return {};
    }
    if (name == "buildUpdateNodes") {
        each(self.get("nodes"), [&](const JSValue& node) {
            if (!node.get("updateType").strictEquals("none")) invoke(self.get("updateNodes"), "push")({node});
        });
        each(self.get("sequentialNodes"), [&](const JSValue& node) {
            const auto before = node.get("updateBeforeType"), after = node.get("updateAfterType");
            if (!before.strictEquals("none")) invoke(self.get("updateBeforeNodes"), "push")({node});
            if (!after.strictEquals("none")) invoke(self.get("updateAfterNodes"), "push")({node});
        }); return {};
    }
    if (name == "isFilteredTexture") {
        for (const auto* property : {"magFilter", "minFilter"})
            for (const auto filter : {LinearFilter, LinearMipmapNearestFilter, NearestMipmapLinearFilter, LinearMipmapLinearFilter})
                if (a.get(property).strictEquals(filter)) return true;
        return false;
    }
    if (name == "getUniformBufferLimit") return invoke(self.get("renderer").get("backend").get("capabilities"), "getUniformBufferLimit")();
    if (name == "addChain") {
        if (!invoke(self.get("chaining"), "indexOf")({a}).strictEquals(-1)) ThreeUtils::warn({"Recursive node: ", a});
        invoke(self.get("chaining"), "push")({a}); return {};
    }
    if (name == "removeChain") {
        if (!invoke(self.get("chaining"), "pop")().strictEquals(a)) throw std::runtime_error("THREE.NodeBuilder: Invalid node chaining!");
        return {};
    }
    if (name == "getMethod") return a;
    if (name == "getTernary" || name == "getFunctionOperator") return nullptr;
    if (name == "getVectorSelect") return invoke(self, "getTernary")({a, raw(args, 1), raw(args, 2)});
    if (name == "getNodeFromHash") return self.get("hashNodes").get(a);
    if (name == "addFlow") { invoke(self.get("flowNodes").get(a), "push")({raw(args, 1)}); return raw(args, 1); }
    if (name == "setContext" || name == "setCache" || name == "setShaderStage" || name == "setBuildStage") {
        const char* field = name == "setContext" ? "context" : name == "setCache" ? "cache" : name == "setShaderStage" ? "shaderStage" : "buildStage";
        self.set(field, a); return {};
    }
    if (name == "getContext" || name == "getCache" || name == "getShaderStage" || name == "getBuildStage")
        return self.get(name == "getContext" ? "context" : name == "getCache" ? "cache" : name == "getShaderStage" ? "shaderStage" : "buildStage");
    if (name == "addContext") {
        const auto previous = invoke(self, "getContext")();
        invoke(self, "setContext")({spread({self.get("context"), a})}); return previous;
    }
    if (name == "getSharedContext") {
        auto context = spread({self.get("context")});
        for (const auto* field : {"material", "getUV", "getOutput", "getTextureLevel", "getAO", "getGI", "getShadow", "nodeLoop", "nodeBlock"}) context.erase(field);
        return context;
    }
    if (name == "getCacheFromNode") {
        auto data = invoke(self, "getDataFromNode")({a});
        if (data.get("cache").isUndefined()) {
            const auto parent = arg(args, 1, true).toBoolean() ? invoke(self, "getCache")() : JSValue(nullptr);
            data.set("cache", nativeNodeCacheConstructor().construct({parent}));
        }
        return data.get("cache");
    }
    if (name == "isAvailable" || name == "isFlipY" || name == "isReservedKeyword" || name == "needsToWorkingColorSpace") return false;
    if (name == "isContextAssign") return self.get("context").get("assign").strictEquals(true);
    if (name == "increaseUsage") {
        auto data = invoke(self, "getDataFromNode")({a});
        data.set("usageCount", data.get("usageCount").isUndefined() ? JSValue(1) : add(data.get("usageCount"), 1));
        const auto field = invoke(self, "isContextAssign")().toBoolean() ? "writeUsageCount" : "readUsageCount";
        data.set(field, data.get(field).isUndefined() ? JSValue(1) : add(data.get(field), 1));
        return data.get("usageCount");
    }
    if (name == "hasWriteUsage") {
        const auto ref = invoke(a, "getShared")({self});
        const auto cache = invoke(ref, "isGlobal")({self}).toBoolean() ? self.get("globalCache") : self.get("cache");
        const auto data = invoke(cache, "getData")({ref});
        if (!data.isUndefined()) for (const auto& stage : forIn(data)) if (checkWriteUsage(data.get(stage))) return true;
        return false;
    }
    if (name == "generateArrayDeclaration") return add(add(add(invoke(self, "getType")({a}), "[ "), raw(args, 1)), " ]");
    if (name == "generateArray") {
        const auto count = raw(args, 1), values = arg(args, 2, nullptr);
        auto snippet = add(invoke(self, "generateArrayDeclaration")({a, count}), "( ");
        for (double i = 0; i < count.toNumber(); ++i) {
            const auto value = values.toBoolean() ? values.get(JSValue(i)) : JSValue(nullptr);
            snippet = add(snippet, !value.isNull() ? invoke(value, "build")({self, a}) : invoke(self, "generateConst")({a}));
            if (i < count.toNumber() - 1) snippet = add(snippet, ", ");
        }
        return add(snippet, " )");
    }
    if (name == "generateStruct") {
        auto snippets = JSValue::array(); const auto values = arg(args, 2, nullptr);
        each(raw(args, 1), [&](const JSValue& member) {
            const auto memberName = member.get("name"), type = member.get("type");
            invoke(snippets, "push")({values.toBoolean() && values.get(memberName).toBoolean() && values.get(memberName).get("isNode").toBoolean() ?
                invoke(values.get(memberName), "build")({self, type}) : invoke(self, "generateConst")({type})});
        });
        return a.toString() + "( " + invoke(snippets, "join")({", "}).toString() + " )";
    }
    if (name == "generateConst") return generateConst(self, args);
    if (name == "getType") return a.strictEquals("color") ? JSValue("vec3") : a;
    if (name == "hasGeometryAttribute") {
        const auto geometry = self.get("geometry");
        return geometry.toBoolean() ? JSValue(!invoke(self.get("geometry"), "getAttribute")({a}).isUndefined()) : geometry;
    }
    if (name == "getPropertyName") return a.get("name");
    if (name == "isScalar") return a.strictEquals("float") || a.strictEquals("bool") || a.strictEquals("int") || a.strictEquals("uint");
    if (name == "isVector" || name == "isMatrix" || name == "isInteger")
        return std::regex_search(a.toString(), std::regex(name == "isVector" ? "vec[0-9]" : name == "isMatrix" ? "mat[0-9]" : "int|uint|(i|u)vec"));
    if (name == "isReference") {
        for (const auto* type : {"void", "property", "sampler", "samplerComparison", "texture", "cubeTexture", "storageTexture", "depthTexture", "texture3D"})
            if (a.strictEquals(type)) return true;
        return false;
    }
    if (name == "getComponentTypeFromTexture") {
        const auto type = a.get("type");
        if (a.get("isDepthTexture").strictEquals(true)) return "float";
        if (type.strictEquals(IntType)) return "int";
        if (type.strictEquals(UnsignedIntType)) return "uint";
        return "float";
    }
    if (name == "getElementType") {
        if (a.strictEquals("mat2")) return "vec2";
        if (a.strictEquals("mat3")) return "vec3";
        if (a.strictEquals("mat4")) return "vec4";
        return invoke(self, "getComponentType")({a});
    }
    if (name == "getComponentType") {
        const auto type = invoke(self, "getVectorType")({a});
        if (type.strictEquals("float") || type.strictEquals("bool") || type.strictEquals("int") || type.strictEquals("uint")) return type;
        const auto text = type.toString(); std::smatch match;
        if (!std::regex_search(text, match, std::regex("(b|i|u|)(vec|mat)([2-4])"))) return nullptr;
        const auto prefix = match[1].str(); return prefix == "b" ? "bool" : prefix == "i" ? "int" : prefix == "u" ? "uint" : "float";
    }
    if (name == "getVectorType") {
        if (a.strictEquals("color")) return "vec3";
        if (a.strictEquals("texture") || a.strictEquals("cubeTexture") || a.strictEquals("storageTexture") || a.strictEquals("texture3D")) return "vec4";
        return a;
    }
    if (name == "getTypeFromLength") return getTypeFromLength(self, args);
    if (name == "getTypeFromArray") {
        const auto type = a.get("constructor");
        for (unsigned i = 0; i < 7; ++i) if (type.strictEquals(captures[1].get("arrayConstructors").get(JSValue(i))))
            return i == 6 ? "float" : i % 2 == 0 ? "int" : "uint";
        return {};
    }
    if (name == "getTypeLength") {
        const auto type = invoke(self, "getVectorType")({a});
        const auto text = type.toString(); std::smatch match;
        if (std::regex_search(text, match, std::regex("vec([2-4])"))) return std::stoi(match[1].str());
        if (type.strictEquals("float") || type.strictEquals("bool") || type.strictEquals("int") || type.strictEquals("uint")) return 1;
        for (const auto [token, count] : {std::pair{"mat2", 4}, {"mat3", 9}, {"mat4", 16}})
            if (a.toString().find(token) != std::string::npos) return count;
        return 0;
    }
    if (name == "getVectorFromMatrix") return invoke(a, "replace")({"mat", "vec"});
    if (name == "changeComponentType") return invoke(self, "getTypeFromLength")({invoke(self, "getTypeLength")({a}), raw(args, 1)});
    if (name == "getPromotedComponentType") {
        JSValue type(nullptr); bool hasWeak = false, fitsUint = true, fitsInt = true;
        const auto rank = [](const JSValue& value) { return value.strictEquals("bool") ? 0 : value.strictEquals("uint") ? 1 : value.strictEquals("int") ? 2 : value.strictEquals("float") ? 3 : -1; };
        for (const auto& node : args) {
            if (node.isNull()) continue;
            if (node.get("isWeak").strictEquals(true)) {
                const auto value = std::trunc(node.get("value").toNumber()); hasWeak = true;
                fitsUint = fitsUint && value >= 0 && value <= 4294967295.0;
                fitsInt = fitsInt && value >= -2147483648.0 && value <= 2147483647.0;
            } else {
                const auto component = invoke(self, "getComponentType")({invoke(node, "getNodeType")({self})});
                if (type.isNull() || rank(component) > rank(type)) type = component;
            }
        }
        if (type.isNull() || (type.strictEquals("bool") && hasWeak)) return "float";
        if (type.strictEquals("uint") && !fitsUint) return fitsInt ? "int" : "float";
        if (type.strictEquals("int") && !fitsInt) return "float";
        return type;
    }
    if (name == "getIntegerType") {
        const auto component = invoke(self, "getComponentType")({a});
        return component.strictEquals("int") || component.strictEquals("uint") ? a : invoke(self, "changeComponentType")({a, "int"});
    }
    if (name == "setActiveStack") { invoke(self.get("activeStacks"), "push")({a}); return {}; }
    if (name == "removeActiveStack") {
        const auto stacks = self.get("activeStacks");
        if (stacks.get(JSValue(self.get("activeStacks").get("length").toNumber() - 1)).strictEquals(a)) invoke(self.get("activeStacks"), "pop")();
        else throw std::runtime_error("THREE.NodeBuilder: Invalid active stack removal.");
        return {};
    }
    if (name == "getActiveStack" || name == "getBaseStack") {
        const auto stacks = self.get("activeStacks");
        return stacks.get(name == "getBaseStack" ? JSValue(0) : JSValue(self.get("activeStacks").get("length").toNumber() - 1));
    }
    if (name == "addStack") {
        self.set("stack", nativeStackNodeConstructor().construct({self.get("stack")}));
        const auto previous = nativeGetCurrentStack(); invoke(self.get("stacks"), "push")({previous});
        nativeSetCurrentStack(self.get("stack")); return self.get("stack");
    }
    if (name == "removeStack") {
        const auto previous = self.get("stack");
        each(previous.get("nodes"), [&](const JSValue& node) { put(invoke(self, "getDataFromNode")({node}), "stack", previous); });
        self.set("stack", previous.get("parent")); nativeSetCurrentStack(invoke(self.get("stacks"), "pop")()); return previous;
    }
    if (name == "getDataFromNode") return getDataFromNode(self, args);
    if (name == "getNodeProperties") {
        auto data = invoke(self, "getDataFromNode")({a, arg(args, 1, "any")});
        auto properties = data.get("properties");
        if (!properties.toBoolean()) { properties = object({{"outputNode", nullptr}}); data.set("properties", properties); }
        return properties;
    }
    if (name == "getAttribute") {
        const auto attributes = self.get("attributes"); auto iterator = getJSIterator(attributes);
        try { while (const auto attribute = stepJSIterator(iterator)) if (attribute->get("name").strictEquals(a)) { closeJSIterator(iterator); return *attribute; } }
        catch (...) { closeJSIterator(iterator, std::current_exception()); }
        const auto attribute = nativeNodeAttributeConstructor().construct({a, raw(args, 1)});
        invoke(self, "registerDeclaration")({attribute}); invoke(attributes, "push")({attribute}); return attribute;
    }
    if (name == "getBufferAttributeFromNode") {
        auto data = invoke(self, "getDataFromNode")({a, "vertex"}); auto attribute = data.get("bufferAttribute");
        if (attribute.isUndefined()) {
            auto uniforms = self.get("uniforms"); const auto index = uniforms.get("index").toNumber(); uniforms.set("index", index + 1);
            auto attributeName = arg(args, 2, nullptr); if (attributeName.isNull()) attributeName = "nodeAttribute" + JSValue(index).toString();
            attribute = nativeNodeAttributeConstructor().construct({attributeName, raw(args, 1), a});
            invoke(self.get("bufferAttributes"), "push")({attribute}); data.set("bufferAttribute", attribute);
        }
        return attribute;
    }
    if (name == "getStructTypeNode") {
        const auto stage = args.size() > 1 && !args[1].isUndefined() ? args[1] : self.get("shaderStage");
        const auto result = self.get("types").get(stage).get(a); return result.toBoolean() ? result : JSValue(nullptr);
    }
    if (name == "getStructTypeFromNode") {
        const auto stage = args.size() > 3 && !args[3].isUndefined() ? args[3] : self.get("shaderStage");
        auto data = invoke(self, "getDataFromNode")({a, stage, self.get("globalCache")}); auto type = data.get("structType");
        if (type.isUndefined()) {
            auto structs = self.get("structs"); const auto index = structs.get("index").toNumber(); structs.set("index", index + 1);
            auto typeName = arg(args, 2, nullptr); if (typeName.isNull()) typeName = "StructType" + JSValue(index).toString();
            type = nativeStructTypeConstructor().construct({typeName, raw(args, 1)});
            invoke(self.get("structs").get(stage), "push")({type}); put(self.get("types").get(stage), typeName, a); data.set("structType", type);
        }
        return type;
    }
    if (name == "getOutputStructTypeFromNode") {
        auto type = invoke(self, "getStructTypeFromNode")({a, raw(args, 1), "OutputType", "fragment"}); type.set("output", true); return type;
    }
    if (name == "getUniformFromNode") {
        const auto stage = args.size() > 2 && !args[2].isUndefined() ? args[2] : self.get("shaderStage");
        auto data = invoke(self, "getDataFromNode")({a, stage, self.get("globalCache")}); auto uniform = data.get("uniform");
        if (uniform.isUndefined()) {
            auto uniforms = self.get("uniforms"); const auto index = uniforms.get("index").toNumber(); uniforms.set("index", index + 1);
            const auto uniformName = arg(args, 3, nullptr);
            uniform = nativeNodeUniformConstructor().construct({uniformName.toBoolean() ? uniformName : JSValue("nodeUniform" + JSValue(index).toString()), raw(args, 1), a});
            invoke(self.get("uniforms").get(stage), "push")({uniform}); invoke(self, "registerDeclaration")({uniform}); data.set("uniform", uniform);
        }
        return uniform;
    }
    if (name == "getVarFromNode") {
        auto varName = arg(args, 1, nullptr);
        const auto type = args.size() > 2 && !args[2].isUndefined() ? args[2] : invoke(a, "getNodeType")({self});
        const auto stage = args.size() > 3 && !args[3].isUndefined() ? args[3] : self.get("shaderStage");
        const auto readOnly = arg(args, 4, false), local = arg(args, 5, false), property = arg(args, 6, "variable");
        auto data = invoke(self, "getDataFromNode")({a, stage});
        const auto subProperty = invoke(self, "getSubBuildProperty")({property, data.get("subBuilds")}); auto variable = data.get(subProperty);
        if (variable.isUndefined()) {
            const auto idNS = readOnly.toBoolean() ? "_const" : "_var";
            auto vars = self.get("vars").get(stage); if (!vars.toBoolean()) { vars = JSValue::array(); put(self.get("vars"), stage, vars); }
            auto id = self.get("vars").get(idNS); if (!id.toBoolean()) { id = 0; put(self.get("vars"), idNS, id); }
            if (varName.isNull()) { varName = add(readOnly.toBoolean() ? JSValue("nodeConst") : JSValue("nodeVar"), id); put(self.get("vars"), idNS, self.get("vars").get(idNS).toNumber() + 1); }
            if (!subProperty.strictEquals(property)) varName = invoke(self, "getSubBuildProperty")({varName, data.get("subBuilds")});
            const auto count = invoke(a, "getArrayCount")({self});
            variable = nativeNodeVarConstructor().construct({varName, type, readOnly, count, local});
            if (!readOnly.toBoolean() && !local.toBoolean()) invoke(vars, "push")({variable});
            invoke(self, "registerDeclaration")({variable}); data.set(subProperty, variable);
        }
        return variable;
    }
    if (name == "getVaryingFromNode") {
        auto varyingName = arg(args, 1, nullptr);
        const auto type = args.size() > 2 && !args[2].isUndefined() ? args[2] : invoke(a, "getNodeType")({self});
        auto data = invoke(self, "getDataFromNode")({a, "any"});
        const auto property = invoke(self, "getSubBuildProperty")({"varying", data.get("subBuilds")}); auto varying = data.get(property);
        if (varying.isUndefined()) {
            const auto varyings = self.get("varyings"); const auto index = varyings.get("length");
            if (varyingName.isNull()) varyingName = add("nodeVarying", index);
            if (!property.strictEquals("varying")) varyingName = invoke(self, "getSubBuildProperty")({varyingName, data.get("subBuilds")});
            varying = nativeNodeVaryingConstructor().construct({varyingName, type, arg(args, 3, nullptr), arg(args, 4, nullptr)});
            invoke(varyings, "push")({varying}); invoke(self, "registerDeclaration")({varying}); data.set(property, varying);
        }
        return varying;
    }
    if (name == "registerDeclaration") {
        const auto stage = self.get("shaderStage"); auto declarations = self.get("declarations").get(stage);
        if (!declarations.toBoolean()) { declarations = object(); put(self.get("declarations"), stage, declarations); }
        const auto keywords = self.get("renderer").get("debug").get("diagnostics").get("keywords");
        const auto base = a.get("name"); auto declarationName = base; auto property = invoke(self, "getPropertyName")({a}); unsigned index = 1;
        while ((keywords.toBoolean() && invoke(self, "isReservedKeyword")({declarationName}).toBoolean()) || !declarations.get(property).isUndefined()) {
            declarationName = add(add(base, "_"), index++); put(a, "name", declarationName); property = invoke(self, "getPropertyName")({a});
        }
        if (!declarationName.strictEquals(base)) ThreeUtils::warn({"TSL: Declaration name '" + base.toString() + "' of '" + a.get("type").toString() + "' is a reserved keyword or already in use. Renamed to '" + declarationName.toString() + "'."});
        declarations.set(property, a); return {};
    }
    if (name == "getCodeFromNode") {
        const auto stage = args.size() > 2 && !args[2].isUndefined() ? args[2] : self.get("shaderStage");
        auto data = invoke(self, "getDataFromNode")({a}); auto code = data.get("code");
        if (code.isUndefined()) {
            auto codes = self.get("codes").get(stage); if (!codes.toBoolean()) { codes = JSValue::array(); put(self.get("codes"), stage, codes); }
            code = nativeNodeCodeConstructor().construct({add("nodeCode", codes.get("length")), raw(args, 1)}); invoke(codes, "push")({code}); data.set("code", code);
        }
        return code;
    }
    if (name == "isDeterministic") {
        auto node = a; if (node.get("isVarNode").toBoolean() && node.get("intent").toBoolean()) node = node.get("node");
        if (node.get("isMathNode").toBoolean()) {
            if (!invoke(self, "isDeterministic")({node.get("aNode")}).toBoolean()) return false;
            if (node.get("bNode").toBoolean() && !invoke(self, "isDeterministic")({node.get("bNode")}).toBoolean()) return false;
            return !node.get("cNode").toBoolean() || invoke(self, "isDeterministic")({node.get("cNode")}).toBoolean();
        }
        if (node.get("isOperatorNode").toBoolean()) return invoke(self, "isDeterministic")({node.get("aNode")}).toBoolean() &&
            (!node.get("bNode").toBoolean() || invoke(self, "isDeterministic")({node.get("bNode")}).toBoolean());
        if (node.get("isArrayNode").toBoolean()) {
            if (!node.get("values").isNull()) {
                auto iterator = getJSIterator(node.get("values"));
                try { while (const auto child = stepJSIterator(iterator)) if (!invoke(self, "isDeterministic")({*child}).toBoolean()) { closeJSIterator(iterator); return false; } }
                catch (...) { closeJSIterator(iterator, std::current_exception()); }
            }
            return true;
        }
        return node.get("isConstNode").toBoolean();
    }
    if (name == "addLineFlowCode") {
        if (a.strictEquals("")) return self;
        auto code = add(self.get("tab"), a);
        if (!std::regex_search(code.toString(), std::regex(";\\s*$"))) code = add(code, ";\n");
        auto flow = self.get("flow"); flow.set("code", add(flow.get("code"), code)); return self;
    }
    if (name == "addFlowCode") { auto flow = self.get("flow"); flow.set("code", add(flow.get("code"), a)); return self; }
    if (name == "addFlowTab") { self.set("tab", add(self.get("tab"), "\t")); return self; }
    if (name == "removeFlowTab") { self.set("tab", invoke(self.get("tab"), "slice")({0, -1})); return self; }
    if (name == "getFlowData") return invoke(self.get("flowsData"), "get")({a});
    if (name == "flowNode") {
        const auto output = invoke(a, "getNodeType")({self}); const auto data = invoke(self, "flowChildNode")({a, output});
        invoke(self.get("flowsData"), "set")({a, data}); return data;
    }
    if (name == "addInclude") {
        if (!self.get("currentFunctionNode").isNull()) invoke(self.get("currentFunctionNode").get("includes"), "push")({a});
        return {};
    }
    if (name == "buildFunctionNode") {
        const auto backend = self.get("renderer").get("backend");
        const auto functionNodeCache = captures[1].get("functionNodeCache");
        auto cache = invoke(functionNodeCache, "get")({backend});
        if (cache.isUndefined()) {
            cache = createNativeWeakMap(); invoke(functionNodeCache, "set")({backend, cache});
        }
        auto function = invoke(cache, "get")({a});
        if (function.isUndefined()) {
            function = nativeFunctionNodeConstructor().construct();
            const auto previous = self.get("currentFunctionNode"); self.set("currentFunctionNode", function);
            function.set("code", invoke(self, "buildFunctionCode")({a}));
            self.set("currentFunctionNode", previous);
            invoke(cache, "set")({a, function});
        }
        return function;
    }
    if (name == "flowShaderNode") {
        const auto layout = a.get("layout");
        auto inputs = object();
        inputs.set(JSValue::wellKnownSymbol("iterator"), JSValue::function([](const JSValue& receiver, const Args&, const Args&) {
            const auto values = invoke(JSValue::intrinsicObjectConstructor(), "values")({receiver});
            auto state = object({{"index", 0}, {"values", values}});
            auto iterator = object();
            iterator.set("next", JSValue::function([](const JSValue&, const Args&, const Args& captures) {
                auto state = captures[0]; const auto values = state.get("values");
                const auto value = values.get(state.get("index"));
                const auto index = state.get("index").toNumber(); state.set("index", index + 1);
                return object({{"value", value}, {"done", index >= values.get("length").toNumber()}});
            }, {state}, "next", 0));
            return iterator;
        }, {}, "[Symbol.iterator]", 0));
        each(layout.get("inputs"), [&](const JSValue& input) {
            const auto name = input.get("name");
            const auto type = input.get("type"), parameterName = input.get("name");
            inputs.set(name, nativeParameterNodeConstructor().construct({type, parameterName}));
        });
        put(a, "layout", nullptr);
        const auto callNode = invoke(a, "call")({inputs});
        const auto flowData = invoke(self, "flowStagesNode")({callNode, layout.get("type")});
        put(a, "layout", layout);
        return flowData;
    }
    if (name == "flowBuildStage") {
        const auto previous = invoke(self, "getBuildStage")(); invoke(self, "setBuildStage")({raw(args, 1)});
        const auto result = invoke(a, "build")({self, arg(args, 2, nullptr)}); invoke(self, "setBuildStage")({previous}); return result;
    }
    if (name == "flowStagesNode") {
        const auto output = arg(args, 1, nullptr);
        const auto previousFlow = self.get("flow"), previousVars = self.get("vars"), previousDeclarations = self.get("declarations"), previousCache = self.get("cache");
        const auto previousStage = self.get("buildStage"), previousStack = self.get("stack"), previousBlock = self.get("flowBlock");
        auto flow = object({{"code", ""}}); self.set("flow", flow); self.set("vars", object()); self.set("declarations", object());
        self.set("cache", nativeNodeCacheConstructor().construct()); self.set("stack", nativeStackNodeConstructor().construct()); self.set("flowBlock", nullptr);
        for (const auto* stage : {"setup", "analyze", "generate"}) { invoke(self, "setBuildStage")({stage}); flow.set("result", invoke(a, "build")({self, output})); }
        flow.set("vars", invoke(self, "getVars")({self.get("shaderStage")}));
        self.set("flow", previousFlow); self.set("vars", previousVars); self.set("declarations", previousDeclarations); self.set("cache", previousCache);
        self.set("stack", previousStack); self.set("flowBlock", previousBlock); invoke(self, "setBuildStage")({previousStage}); return flow;
    }
    if (name == "flowChildNode") {
        const auto previous = self.get("flow"); auto flow = object({{"code", ""}}); self.set("flow", flow);
        flow.set("result", invoke(a, "build")({self, arg(args, 1, nullptr)})); self.set("flow", previous); return flow;
    }
    if (name == "flowNodeFromShaderStage") {
        const auto node = raw(args, 1), output = arg(args, 2, nullptr), property = arg(args, 3, nullptr);
        const auto previousTab = self.get("tab"), previousCache = self.get("cache"), previousStage = self.get("shaderStage"), previousContext = self.get("context"), previousBlock = self.get("flowBlock");
        invoke(self, "setShaderStage")({a}); auto context = spread({self.get("context")}); context.erase("nodeBlock");
        self.set("flowBlock", nullptr); self.set("cache", self.get("globalCache")); self.set("tab", "\t"); self.set("context", context);
        JSValue result(nullptr);
        if (self.get("buildStage").strictEquals("generate")) {
            auto data = invoke(self, "flowChildNode")({node, output});
            if (!property.isNull()) data.set("code", add(data.get("code"), add(self.get("tab"), property).toString() + " = " + data.get("result").toString() + ";\n"));
            put(self.get("flowCode"), a, add(self.get("flowCode").get(a), data.get("code"))); result = data;
        } else result = invoke(node, "build")({self});
        invoke(self, "setShaderStage")({previousStage}); self.set("cache", previousCache); self.set("tab", previousTab);
        self.set("context", previousContext); self.set("flowBlock", previousBlock); return result;
    }
    if (name == "getAttributesArray") return invoke(self.get("attributes"), "concat")({self.get("bufferAttributes")});
    if (name == "getVar") {
        const auto count = arg(args, 2, nullptr);
        return (!count.isNull() ? invoke(self, "generateArrayDeclaration")({a, count}) : invoke(self, "getType")({a})).toString() + " " + raw(args, 1).toString();
    }
    if (name == "generateConstStatement") return "const " + invoke(self, "getVar")({a, raw(args, 1), arg(args, 2, nullptr)}).toString();
    if (name == "generateVarStatement") return invoke(self, "getVar")({a, raw(args, 1), arg(args, 2, nullptr)});
    if (name == "generateLetStatement") return invoke(self, "generateVarStatement")({a, raw(args, 1), arg(args, 2, nullptr)});
    if (name == "getVars") {
        auto snippets = JSValue::array(); const auto vars = self.get("vars").get(a);
        if (!vars.isUndefined()) each(vars, [&](const JSValue& var) {
            invoke(snippets, "push")({invoke(self, "getVar")({var.get("type"), var.get("name"), var.get("count")}).toString() + ";"});
        });
        return invoke(snippets, "join")({arg(args, 1, false).toBoolean() ? "\n" : "\n\t"});
    }
    if (name == "getCodes") {
        const auto codes = self.get("codes").get(a); JSValue result("");
        if (!codes.isUndefined()) each(codes, [&](const JSValue& code) { result = add(result, add(code.get("code"), "\n")); });
        return result;
    }
    if (name == "getHash") {
        const auto vertex = self.get("vertexShader"), fragment = self.get("fragmentShader");
        const auto first = add(vertex, fragment); const auto compute = self.get("computeShader"); return add(first, compute);
    }
    if (name == "addSubBuild") { invoke(self.get("subBuildLayers"), "push")({a}); return {}; }
    if (name == "removeSubBuild") return invoke(self.get("subBuildLayers"), "pop")();
    if (name == "getClosestSubBuild") return closestSubBuild(self, a);
    if (name == "getSubBuildOutput") return invoke(self, "getSubBuildProperty")({"outputNode", a});
    if (name == "getSubBuildProperty") {
        const auto property = arg(args, 0, ""), node = arg(args, 1, nullptr);
        const auto subBuild = !node.isNull() ? invoke(self, "getClosestSubBuild")({node}) : self.get("subBuildFn");
        return subBuild.toBoolean() ? (property.toBoolean() ? add(add(subBuild, "_"), property) : subBuild) : property;
    }
    if (name == "prebuild") {
        const auto renderer = self.get("renderer");
        const auto material = self.get("material");
        if (renderer.get("contextNode").get("isContextNode").strictEquals(true)) {
            auto context = spread({self.get("context")});
            const auto data = invoke(renderer.get("contextNode"), "getFlowContextData")();
            spreadInto(context, data); self.set("context", context);
        } else ThreeUtils::error({"NodeBuilder: \"renderer.contextNode\" must be an instance of `context()`."});
        if (material.toBoolean() && material.get("contextNode").toBoolean()) {
            if (material.get("contextNode").get("isContextNode").strictEquals(true)) {
                auto context = spread({self.get("context")});
                const auto data = invoke(material.get("contextNode"), "getFlowContextData")();
                spreadInto(context, data); self.set("context", context);
            } else ThreeUtils::error({"NodeBuilder: \"material.contextNode\" must be an instance of `context()`."});
        }
        if (!material.isNull()) {
            const auto nodeMaterial = invoke(renderer.get("library"), "fromMaterial")({material});
            if (nodeMaterial.isNull()) {
                ThreeUtils::error({"NodeBuilder: Material \"" + material.get("type").toString() + "\" is not compatible."});
                throw std::logic_error("NodeBuilder fallback requires the pending NodeMaterial implementation");
            }
            invoke(nodeMaterial, "build")({self});
        } else invoke(self, "addFlow")({"compute", self.get("compute")});
        return {};
    }
    if (name == "buildAsync") {
        const auto state = captures[1];
        return buildNodeBuilderAsync(self, arg(args, 0, state.get("yieldToMain")), state.get("buildStages"), state.get("shaderStages"));
    }
    if (name == "build") {
        invoke(self, "prebuild")();
        each(captures[1].get("buildStages"), [&](const JSValue& stage) {
            invoke(self, "setBuildStage")({stage});
            if (self.get("context").get("position").toBoolean() && self.get("context").get("position").get("isNode").toBoolean())
                invoke(self, "flowNodeFromShaderStage")({"vertex", self.get("context").get("position")});
            each(captures[1].get("shaderStages"), [&](const JSValue& shaderStage) {
                invoke(self, "setShaderStage")({shaderStage});
                each(self.get("flowNodes").get(shaderStage), [&](const JSValue& node) {
                    if (stage.strictEquals("generate")) invoke(self, "flowNode")({node}); else invoke(node, "build")({self});
                });
            });
        });
        invoke(self, "setBuildStage")({nullptr}); invoke(self, "setShaderStage")({nullptr});
        invoke(self, "buildCode")(); invoke(self, "buildUpdateNodes")(); return self;
    }
    if (name == "format") return format(self, args);
    if (name == "getNodeUniform") {
        const auto type = raw(args, 1);
        const char* constructorName = nullptr;
        if (type.strictEquals("float") || type.strictEquals("int") || type.strictEquals("uint")) constructorName = "NumberNodeUniform";
        else if (type.strictEquals("vec2") || type.strictEquals("ivec2") || type.strictEquals("uvec2")) constructorName = "Vector2NodeUniform";
        else if (type.strictEquals("vec3") || type.strictEquals("ivec3") || type.strictEquals("uvec3")) constructorName = "Vector3NodeUniform";
        else if (type.strictEquals("vec4") || type.strictEquals("ivec4") || type.strictEquals("uvec4")) constructorName = "Vector4NodeUniform";
        else if (type.strictEquals("color")) constructorName = "ColorNodeUniform";
        else if (type.strictEquals("mat2")) constructorName = "Matrix2NodeUniform";
        else if (type.strictEquals("mat3")) constructorName = "Matrix3NodeUniform";
        else if (type.strictEquals("mat4")) constructorName = "Matrix4NodeUniform";
        if (constructorName) return nativeNodeUniforms().get(constructorName).construct({a});
        throw std::runtime_error("THREE.NodeBuilder: Uniform \"" + type.toString() + "\" not implemented.");
    }
    if (name == "getSignature") return std::string("// Three.js r") + REVISION + " - Node System\n";
    if (name == "needsPreviousData") { const auto mrt = invoke(self.get("renderer"), "getMRT")(); return !mrt.isNull() && invoke(mrt, "has")({"velocity"}).toBoolean(); }
    // These are abstract extension points in the upstream base class itself.
    ThreeUtils::warn({"Abstract function."}); return {};
}
void method(JSValue prototype, const char* name, unsigned arity, const JSValue& state) {
    JSPropertyDescriptor d; d.value = JSValue::function(dispatch, {name, state}, name, arity);
    d.writable = d.configurable = true; prototype.defineProperty(name, d);
}
void getter(JSValue prototype, const char* name) {
    JSPropertyDescriptor d;
    d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& captures) {
        const auto& key = captures[0].string();
        if (key == "renderPipeline") return self.get("context").get("renderPipeline");
        const auto array = self.get(key == "subBuild" ? "subBuildLayers" : "chaining");
        const auto value = array.get(JSValue(self.get(key == "subBuild" ? "subBuildLayers" : "chaining").get("length").toNumber() - 1));
        return key == "subBuild" && !value.toBoolean() ? JSValue(nullptr) : value;
    }, {name}, std::string("get ") + name, 0);
    d.configurable = true; prototype.defineProperty(name, d);
}
} // namespace

JSValue nativeNodeBuilderConstructor() {
    static thread_local JSWeakValue weak;
    if (auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeBuilder constructor");
    // NodeMaterial imports alpha hashing and TSLBase. Load their real ported
    // dependencies; the remaining NodeMaterial graph is still incomplete.
    nativeAlphaHashThreshold();
    auto state = JSValue::object(nullptr); state.set("id", 0);
    state.set("functionNodeCache", createNativeWeakMap());
    auto arrayConstructors = JSValue::array();
    for (unsigned i = 0; i < 7; ++i) arrayConstructors.set(std::to_string(i), NativeArray_detail::constructor(i));
    state.set("arrayConstructors", arrayConstructors);
    const auto constants = nativeNodeConstants();
    state.set("buildStages", constants.get("defaultBuildStages")); state.set("shaderStages", constants.get("shaderStages"));
    state.set("yieldToMain", JSValue::function(+[](const JSValue&, const Args&, const Args&) { return ThreeUtils::yieldToMain(); }, {}, "yieldToMain", 0));
    auto constructor = JSValue::constructor(initialize, {state}, "NodeBuilder", 3);
    auto prototype = constructor.get("prototype");
    for (const auto [name, arity] : {
        std::pair{"isFlatShading", 0u}, {"isOpaque", 0u}, {"createRenderTarget", 3u}, {"createCubeRenderTarget", 2u}, {"includes", 1u}, {"getOutputType", 0u}, {"getOutputStructName", 0u},
        {"_getBindGroup", 2u}, {"getBindGroupArray", 2u}, {"getBindings", 0u}, {"sortBindingGroups", 0u},
        {"setHashNode", 2u}, {"addNode", 1u}, {"addSequentialNode", 1u}, {"buildUpdateNodes", 0u}}) method(prototype, name, arity, state);
    getter(prototype, "currentNode"); getter(prototype, "renderPipeline");
    for (const auto [name, arity] : {
        std::pair{"isFilteredTexture", 1u}, {"getUniformBufferLimit", 0u}, {"addChain", 1u}, {"removeChain", 1u},
        {"getMethod", 1u}, {"getTernary", 0u}, {"getVectorSelect", 3u}, {"getNodeFromHash", 1u}, {"addFlow", 2u},
        {"setContext", 1u}, {"getContext", 0u}, {"addContext", 1u}, {"getSharedContext", 0u},
        {"setCache", 1u}, {"getCache", 0u}, {"getCacheFromNode", 1u}, {"isAvailable", 0u},
        {"getVertexIndex", 0u}, {"getInstanceIndex", 0u}, {"getDrawIndex", 0u}, {"getFrontFacing", 0u},
        {"getFragCoord", 0u}, {"getSubgroupSize", 0u}, {"getInvocationSubgroupIndex", 0u}, {"getSubgroupIndex", 0u},
        {"enableSubGroups", 0u}, {"enableShaderF16", 0u}, {"enableDualSourceBlending", 0u},
        {"isFlipY", 0u}, {"isContextAssign", 0u}, {"increaseUsage", 1u}, {"hasWriteUsage", 1u},
        {"generateTexture", 0u}, {"generateTextureLod", 0u}, {"generateTextureSize", 0u},
        {"generateArrayDeclaration", 2u}, {"generateArray", 2u}, {"generateStruct", 2u}, {"generateConst", 1u},
        {"getType", 1u}, {"hasGeometryAttribute", 1u}, {"getAttribute", 2u}, {"getPropertyName", 1u},
        {"isScalar", 1u}, {"isReservedKeyword", 0u}, {"isVector", 1u}, {"isMatrix", 1u}, {"isReference", 1u},
        {"needsToWorkingColorSpace", 0u}, {"getComponentTypeFromTexture", 1u}, {"getElementType", 1u},
        {"getComponentType", 1u}, {"getVectorType", 1u}, {"getTypeFromLength", 1u}, {"getTypeFromArray", 1u}, {"getTypeFromAttribute", 1u},
        {"isInteger", 1u}, {"getTypeLength", 1u}, {"getVectorFromMatrix", 1u}, {"changeComponentType", 2u},
        {"getPromotedComponentType", 0u}, {"getIntegerType", 1u}, {"setActiveStack", 1u}, {"removeActiveStack", 1u},
        {"getActiveStack", 0u}, {"getBaseStack", 0u}, {"addStack", 0u}, {"removeStack", 0u},
        {"getDataFromNode", 1u}, {"getNodeProperties", 1u}, {"getBufferAttributeFromNode", 2u},
        {"getStructTypeNode", 1u}, {"getStructTypeFromNode", 2u}, {"getOutputStructTypeFromNode", 2u},
        {"getUniformFromNode", 2u}, {"getVarFromNode", 1u}, {"isDeterministic", 1u}, {"getVaryingFromNode", 1u},
        {"registerDeclaration", 1u}, {"getCodeFromNode", 2u}, {"addLineFlowCode", 1u}, {"addFlowCode", 1u},
        {"addFlowTab", 0u}, {"removeFlowTab", 0u}, {"getFlowData", 1u}, {"flowNode", 1u}, {"addInclude", 1u}, {"buildFunctionNode", 1u},
        {"flowShaderNode", 1u}, {"flowBuildStage", 2u}, {"flowStagesNode", 1u}, {"getFunctionOperator", 0u}, {"buildFunctionCode", 0u},
        {"flowChildNode", 1u}, {"flowNodeFromShaderStage", 2u}, {"getAttributesArray", 0u}, {"getAttributes", 0u},
        {"getVaryings", 0u}, {"getVar", 2u}, {"generateConstStatement", 2u}, {"generateVarStatement", 2u},
        {"generateLetStatement", 2u}, {"getVars", 1u}, {"getUniforms", 0u}, {"getCodes", 1u}, {"getHash", 0u},
        {"setShaderStage", 1u}, {"getShaderStage", 0u}, {"setBuildStage", 1u}, {"getBuildStage", 0u}, {"buildCode", 0u}}) method(prototype, name, arity, state);
    getter(prototype, "subBuild");
    for (const auto [name, arity] : {
        std::pair{"addSubBuild", 1u}, {"removeSubBuild", 0u}, {"getClosestSubBuild", 1u}, {"getSubBuildOutput", 1u},
        {"getSubBuildProperty", 0u}, {"prebuild", 0u}, {"build", 0u}, {"buildAsync", 0u}, {"getNodeUniform", 2u}, {"format", 3u}, {"getSignature", 0u}, {"needsPreviousData", 0u}}) method(prototype, name, arity, state);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
NodeBuilder::NodeBuilder(JSValue nodeObject, JSValue renderer, JSValue parser) {
    static_cast<void>(JSWeakValue(nodeObject)); static_cast<void>(JSWeakValue(renderer)); static_cast<void>(JSWeakValue(parser));
    object_ = constructor().construct({nodeObject, renderer, parser});
}
JSValue NodeBuilder::object() const { static_cast<void>(JSWeakValue(object_)); return object_; }
JSValue NodeBuilder::constructor() { return nativeNodeBuilderConstructor(); }
JSValue NodeBuilder::prototype() { return constructor().get("prototype"); }
