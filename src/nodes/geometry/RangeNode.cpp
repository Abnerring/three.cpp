#include "nodes/geometry/RangeNode.h"

#include "nodes/core/IndexNode.h"
#include "nodes/core/NodeError.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/core/UniformNode.h"
#include "nodes/math/Hash.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <cstdint>

namespace {
using namespace UtilityNodeRuntime;

JSValue getConstNode(const JSValue& node) {
    auto state = JSValue::object(nullptr);
    const auto callback = JSValue::function(
        [](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto value = arg(args, 0);
            if (value.get("isConstNode").strictEquals(true)) {
                auto state = captures[0];
                state.set("value", value);
            }
            return {};
        },
        {state}, "", 1);
    invoke(node, "traverse")({callback});
    const auto result = state.get("value");
    if (result.isUndefined() || result.isNull())
        throw NodeError("THREE.TSL: No \"ConstNode\" found in node graph.");
    return result;
}

JSValue component(const JSValue& value, std::size_t index, bool scalar) {
    if (scalar) return value;
    if (value.get("isColor").strictEquals(true)) {
        static constexpr const char* names[] = {"r", "g", "b", "a"};
        if (index < 3) return value.get(names[index]);
        return 1;
    }
    static constexpr const char* names[] = {"x", "y", "z", "w"};
    if (index >= 4) return 0;
    const auto result = value.get(names[index]);
    return result.isUndefined() || result.isNull() ? JSValue(0) : result;
}

JSValue vector4FromValue(const JSValue& value, double length) {
    const bool scalar = length == 1;
    auto vec4 = nativeTSLBaseBinding("vec4");
    return vec4.call({}, {component(value, 0, scalar), component(value, 1, scalar),
                          component(value, 2, scalar), component(value, 3, scalar)});
}

JSValue objectSeed() {
    static thread_local JSValue strong;
    if (strong.isObject()) return strong;
    strong = nativeUniformFactory().call({}, {0, "uint"});
    const auto callback = JSValue::function(
        [](const JSValue&, const Args& args, const Args&) -> JSValue {
            const auto event = arg(args, 0);
            const auto object = event.get("object");
            const auto id = object.isNull() || object.isUndefined() ? 0.0 : object.get("id").toNumber();
            const auto value = static_cast<std::uint32_t>(static_cast<std::int64_t>(id)) * 0x9E3779B9u;
            return static_cast<double>(value);
        },
        {}, "", 1);
    invoke(strong, "onObjectUpdate")({callback});
    return strong;
}

JSValue setup(const JSValue& self, const JSValue& builder) {
    const auto minNode = getConstNode(self.get("minNode"));
    const auto maxNode = getConstNode(self.get("maxNode"));
    const auto minValue = minNode.get("value");
    const auto maxValue = maxNode.get("value");
    const auto minLength = invoke(builder, "getTypeLength")({nativeNodeUtilsBinding("getValueType").call({}, {minValue})});
    const auto maxLength = invoke(builder, "getTypeLength")({nativeNodeUtilsBinding("getValueType").call({}, {maxValue})});
    const auto minVec = vector4FromValue(minValue, minLength.toNumber());
    const auto maxVec = vector4FromValue(maxValue, maxLength.toNumber());

    const auto salt = nativeTSLBaseBinding("uint").call({}, {self.get("_salt")});
    const auto seed = invoke(objectSeed(), "bitXor")({salt});
    const auto index = invoke(invoke(nativeIndexNodeBinding("instanceIndex"), "mul")({4}), "add")({seed});
    const auto hash = nativeHashBinding("hash");
    const auto h0 = hash.call({}, {index});
    const auto h1 = hash.call({}, {invoke(index, "add")({1})});
    const auto h2 = hash.call({}, {invoke(index, "add")({2})});
    const auto h3 = hash.call({}, {invoke(index, "add")({3})});
    const auto random = nativeTSLBaseBinding("vec4").call({}, {h0, h1, h2, h3});
    auto mixed = nativeMathNodeBinding("mix").call({}, {minVec, maxVec, random});
    const auto nodeType = invoke(self, "getNodeType")({builder});
    const auto convert = mixed.get("convert");
    if (convert.isCallable()) return convert.call(mixed, {nodeType});
    return nativeTSLCoreBinding("convert").call({}, {mixed, nodeType});
}

JSValue method(const JSValue& self, const Args& args, const Args& captures) {
    const auto name = captures[0].string();
    if (name == "isCacheable") return false;
    if (name == "getVectorLength") {
        const auto builder = arg(args, 0);
        const auto minNode = getConstNode(self.get("minNode"));
        const auto maxNode = getConstNode(self.get("maxNode"));
        const auto getType = nativeNodeUtilsBinding("getValueType");
        const auto minLength = invoke(builder, "getTypeLength")({getType.call({}, {minNode.get("value")})});
        const auto maxLength = invoke(builder, "getTypeLength")({getType.call({}, {maxNode.get("value")})});
        return minLength.toNumber() > maxLength.toNumber() ? minLength : maxLength;
    }
    if (name == "generateNodeType") {
        const auto length = invoke(self, "getVectorLength")({arg(args, 0)});
        return invoke(arg(args, 0), "getTypeFromLength")({length});
    }
    if (name == "getConstNode") return getConstNode(arg(args, 0));
    if (name == "setup") return setup(self, arg(args, 0));
    throw std::logic_error("Unknown RangeNode method");
}

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}
} // namespace

JSValue nativeRangeNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "RangeNode module");
    nativeTSLBase();
    nativeIndexNodeModule();
    nativeHashModule();
    nativeMathNodeModule();

    static thread_local std::uint32_t rangeId = 0;
    auto constructor = makeConstructor(
        "RangeNode", 2,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            auto minNode = arg(args, 0);
            auto maxNode = arg(args, 1);
            if (minNode.isUndefined()) minNode = nativeTSLBaseBinding("float").call();
            if (maxNode.isUndefined()) maxNode = nativeTSLBaseBinding("float").call();
            self.set("minNode", minNode);
            self.set("maxNode", maxNode);
            self.set("_salt", static_cast<double>(rangeId++ * 0x85EBCA6Bu));
            return self;
        },
        {{"isCacheable", 0}, {"getVectorLength", 1}, {"generateNodeType", 1}, {"getConstNode", 1}, {"setup", 1}},
        method);
    const auto proxy = nativeTSLBaseBinding("nodeProxy").call({}, {constructor});
    const auto range = invoke(proxy, "setParameterLength")({2});
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", constructor);
    own(bindings, "range", range);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    auto module = JSValue::object(nullptr);
    own(module, "default", constructor);
    own(module, "range", range);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeRangeNodeBinding(const std::string& name) {
    const auto keeper = nativeRangeNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeRangeNodeConstructor() {
    return nativeRangeNodeBinding("default");
}
