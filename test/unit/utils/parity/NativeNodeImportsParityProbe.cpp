#include "animation/AnimationUtils.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/ContextNode.h"
#include "nodes/core/StackNode.h"
#include "nodes/math/ConditionalNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/display/ColorSpaceFunctions.h"
#include "nodes/functions/material/getAlphaHashThreshold.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <map>

namespace {
using Args = std::vector<JSValue>;
void own(JSValue object, const std::string& key, const JSValue& value) {
    JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; object.defineProperty(key, d);
}
JSValue array(const Args& values = {}) {
    auto result = JSValue::array(); for (const auto& value : values) own(result, std::to_string(result.length()), value); return result;
}
JSValue call(const JSValue& object, const char* key, const Args& args = {}) { return object.get(key).call(object, args); }
JSValue normal(const JSValue& value, std::vector<JSValue>* seen = nullptr, std::map<double, unsigned>* ids = nullptr) {
    if (value.isUndefined()) return "#undefined";
    if (value.isCallable()) return "#function";
    if (value.isSymbol()) return "#symbol";
    if (value.type() == JSValue::Type::Number) {
        if (std::isnan(value.number())) return "#NaN";
        if (std::isinf(value.number())) return value.number() > 0 ? "#Infinity" : "#-Infinity";
        if (value.number() == 0 && std::signbit(value.number())) return "#-0";
    }
    if (!value.isObject()) return value;
    std::vector<JSValue> localSeen; if (!seen) seen = &localSeen;
    for (unsigned i = 0; i < seen->size(); ++i) if (value.sameValue((*seen)[i])) return array({"#ref", i});
    seen->push_back(value);
    std::map<double, unsigned> localIds; if (!ids) ids = &localIds;
    auto result = value.type() == JSValue::Type::Array ? JSValue::array(value.length()) : JSValue::object();
    for (const auto& key : value.keys()) {
        const auto field = value.get(key);
        if (key == "id" && value.get("isNode").strictEquals(true) && field.type() == JSValue::Type::Number) {
            const auto [position, inserted] = ids->emplace(field.number(), static_cast<unsigned>(ids->size()));
            own(result, key, array({"#nodeId", position->second}));
        } else own(result, key, normal(field, seen, ids));
    }
    return result;
}
JSValue module(const std::string& name) {
    if (name == "TSL") return nativeTSLCore();
    if (name == "NodeUtils") return nativeNodeUtils();
    if (name == "Property") return nativePropertyNodeModule();
    if (name == "Conditional") return nativeConditionalNodeModule();
    if (name == "Operator") return nativeOperatorNodeModule();
    if (name == "Math") return nativeMathNodeModule();
    throw std::logic_error("Unknown import source");
}
JSValue behavior(const std::string& target) {
    auto result = JSValue::object();
    if (target == "TSL") {
        const auto exports = nativeTSLCore(); auto values = array();
        for (const auto& value : Args{true,7,0.25,nullptr}) call(values, "push", {normal(exports.get("nodeObject").call({}, {value}))});
        result.set("values", values); result.set("vector", normal(exports.get("vec3").call({}, {1,2,3})));
    } else if (target == "Property") {
        const auto exports = nativePropertyNodeModule();
        const auto property = exports.get("property").call({}, {"vec3", "output", 4}); result.set("property", normal(property)); result.set("cacheKey", call(property, "customCacheKey"));
        result.set("diffuseColor", normal(exports.get("diffuseColor")));
        result.set("ambientOcclusion", normal(exports.get("ambientOcclusion")));
    } else if (target == "Operator") {
        const auto exports = nativeOperatorNodeModule();
        result.set("mul", normal(exports.get("mul").call({}, {2,3,4})));
        result.set("equal", normal(exports.get("equal").call({}, {1,2})));
        result.set("incrementFn", exports.get("increment").get("isFn"));
    } else if (target == "Math") {
        const auto exports = nativeMathNodeModule();
        result.set("pow3", normal(exports.get("pow3").call({}, {2})));
        result.set("clamp", normal(exports.get("clamp").call({}, {0.5})));
        result.set("transform", normal(exports.get("transformNormalByViewMatrix").call({}, {1,2})));
        const auto uv = nativeTSLCoreBinding("vec2").call({}, {0.2,0.4});
        result.set("rand", normal(exports.get("rand").get("shaderNode").get("jsFunc").call({}, {array({uv})})));
        auto builder = JSValue::object(); builder.set("getNodeProperties", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return JSValue::object(); }));
        auto nodes = array();
        for (const auto* name : {"oneMinus", "reciprocal", "difference"}) {
            const auto node = exports.get(name).call({}, {2,3}); call(nodes, "push", {normal(call(node, "setup", {builder}))});
        }
        result.set("setup", nodes);
    } else if (target == "Conditional") {
        const auto exports = nativeConditionalNodeModule();
        result.set("select", normal(exports.get("select").call({}, {true,2,3})));
        auto properties = array(); auto prototype = nativePropertyNodeConstructor().get("prototype"); const auto old = prototype.getOwnPropertyDescriptor("build");
        prototype.set("build", JSValue::function([](const JSValue& self, const Args&, const Args& captures) -> JSValue {
            call(captures[0], "push", {normal(self)}); throw JSTypeError("stop after real property allocation");
        }, {properties}));
        auto node = exports.get("select").call({}, {true,2,3}); node.set("getNodeType", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "float"; }));
        auto builder = JSValue::object(); builder.set("getDataFromNode", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return JSValue::object(); }));
        auto inputs = JSValue::object(); inputs.set("condNode", node.get("condNode")); inputs.set("ifNode", node.get("ifNode")); inputs.set("elseNode", node.get("elseNode"));
        builder.set("getNodeProperties", JSValue::function([](const JSValue&, const Args&, const Args& captures) -> JSValue { return captures[0]; }, {inputs})); builder.set("currentFunctionNode", nullptr);
        try { call(node, "generate", {builder, "float"}); result.set("generate", "returned"); } catch (const JSTypeError&) { result.set("generate", "#TypeError"); }
        if (old) prototype.defineProperty("build", *old); else prototype.erase("build"); result.set("properties", properties);
    } else if (target == "Context") {
        const auto exports = nativeContextNodeModule(); const auto n = nativeNodeConstructor().construct({"float"});
        result.set("context", normal(exports.get("context").call({}, {n})));
        result.set("uniform", normal(call(nativeTSLCoreBinding("nodeObject").call({}, {n}), "uniformFlow")));
    } else if (target == "Stack") {
        const auto exports = nativeStackNodeModule(); auto n = exports.get("stack").call({});
        const auto fn = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
        call(n, "If", {true, fn}); call(n, "ElseIf", {false, fn}); call(n, "Else", {fn}); call(n, "Switch", {2});
        result.set("nodes", normal(n.get("nodes"))); result.set("current", normal(n.get("_currentCond"))); result.set("expression", normal(n.get("_expressionNode")));
    } else if (target == "Color") {
        const auto exports = nativeColorSpaceFunctions(); const auto color = nativeTSLCoreBinding("vec3").call({}, {0.1,0.2,0.3});
        for (const auto* name : {"sRGBTransferEOTF", "sRGBTransferOETF"}) result.set(name, normal(exports.get(name).get("shaderNode").get("jsFunc").call({}, {array({color})})));
    } else if (target == "Alpha") {
        const auto threshold = nativeAlphaHashThreshold(); const auto position = nativeTSLCoreBinding("vec3").call({}, {0.1,0.2,0.3});
        const auto previous = nativeGetCurrentStack(); const auto stack = nativeStackNodeConstructor().construct(); nativeSetCurrentStack(stack);
        try { result.set("result", normal(threshold.get("shaderNode").get("jsFunc").call({}, {array({position})}))); result.set("stack", normal(stack.get("nodes"))); }
        catch (...) { nativeSetCurrentStack(previous); throw; }
        nativeSetCurrentStack(previous);
    } else throw std::logic_error("Unknown import target");
    return result;
}
}
int main() {
    try {
        auto realm = JSValue::object(); const std::string input(std::istreambuf_iterator<char>(std::cin), {}); const auto request = Animation_detail::parseJSON(input);
        auto source = module(request.get("source").string()), state = JSValue::object(); state.set("calls", 0);
        const auto poison = JSValue::function([](const JSValue&, const Args&, const Args& captures) -> JSValue { auto state = captures[0]; state.set("calls", state.get("calls").toNumber()+1); throw JSTypeError("facade export must not be imported"); }, {state});
        auto output = JSValue::object(); output.set("mutationAccepted", source.reflectSet(request.get("key"), poison, source)); output.set("sourceReplaced", source.get(request.get("key")).sameValue(poison));
        auto events = array(); const auto previousConsole = ThreeUtils::getConsoleFunction(); ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&, const Args& args, const Args& captures) -> JSValue { call(captures[0], "push", {normal(array(args))}); return {}; }, {events}));
        try { output.set("behavior", behavior(request.get("target").string())); } catch (const JSTypeError&) { output.set("behavior", "#TypeError"); } catch (const std::exception& e) { output.set("behavior", "#Error"); output.set("nativeError", e.what()); }
        ThreeUtils::setConsoleFunction(previousConsole); output.set("poisonCalls", state.get("calls")); output.set("events", events); std::cout << output.toJSON()->dump();
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
