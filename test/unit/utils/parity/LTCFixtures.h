#pragma once

#include "nodes/core/StackNode.h"
#include "nodes/functions/BSDF/LTC.h"
#include "nodes/tsl/TSLBase.h"

#include <functional>
#include <vector>

namespace LTCFixtures {
using Args = std::vector<JSValue>;
inline JSValue call(const JSValue& value, const char* name, const Args& args = {}) {
    return value.get(name).call(value, args);
}
inline void push(const JSValue& array, const JSValue& value) { call(array, "push", {value}); }
inline JSValue record(std::initializer_list<std::pair<const char*, JSValue>> fields = {}) {
    auto value = JSValue::object();
    for (const auto& [name, field] : fields) value.set(name, field);
    return value;
}
inline JSValue node(const char* type, const Args& args = {}) {
    return nativeTSLBaseBinding(type).call({}, args);
}
struct StackScope {
    JSValue previous = nativeGetCurrentStack(), stack = nativeStackNodeConstructor().construct();
    StackScope() { nativeSetCurrentStack(stack); }
    ~StackScope() { nativeSetCurrentStack(previous); }
};
inline JSValue shape(const JSValue& shader) {
    const auto layout = shader.get("layout");
    return record({{"callbackLength", shader.get("jsFunc").get("length")},
                   {"nodeType", shader.get("nodeType")}, {"layoutType", layout.get("type")},
                   {"inputs", layout.get("inputs")}});
}
inline JSValue namedInputs() {
    return record({{"N", node("vec3", {0, 0, 1})}, {"V", node("vec3", {.2, .1, 1})},
                   {"P", node("vec3", {0, 0, 2})}, {"mInv", node("mat3")},
                   {"p0", node("vec3", {-1, -1, 0})}, {"p1", node("vec3", {1, -1, 0})},
                   {"p2", node("vec3", {1, 1, 0})}, {"p3", node("vec3", {-1, 1, 0})},
                   {"roughness", node("float", {.5})}});
}
inline JSValue uvBody() {
    const auto inputs = namedInputs();
    StackScope scope;
    const auto shader = nativeLTCBinding("LTC_Uv").get("shaderNode");
    const auto result = shader.get("jsFunc").call({}, {inputs});
    return record({{"isNode", result.get("isNode")}, {"type", result.get("type")},
                   {"assignments", scope.stack.get("nodes").length()}});
}
inline JSValue internalShapes() {
    const auto inputs = namedInputs();
    StackScope scope;
    const auto shader = nativeLTCBinding("LTC_Evaluate_Volume").get("shaderNode");
    shader.get("jsFunc").call({}, {inputs});
    // Execute the deferred front-facing branch so both internal typed functions
    // occur in a real graph, rather than duplicating their layouts in the fixture.
    scope.stack.get("_currentCond").get("ifNode").get("jsFunc").call();
    auto shapes = record();
    Args seen;
    std::function<void(const JSValue&)> walk = [&](const JSValue& value) {
        if (!value.isObject() || value.isCallable()) return;
        for (const auto& old : seen) if (old.sameValue(value)) return;
        seen.push_back(value);
        const auto shader = value.get("shaderNode");
        if (shader.isObject() && !shader.get("layout").isNull()) {
            const auto first = shader.get("layout").get("inputs").get("0").get("name");
            if (first.strictEquals("f")) shapes.set("LTC_ClippedSphereFormFactor", shape(shader));
            if (first.strictEquals("v1")) shapes.set("LTC_EdgeVectorFormFactor", shape(shader));
        }
        for (const auto& key : value.keys()) if (key != "stackTrace") walk(value.get(key));
    };
    walk(scope.stack.get("nodes"));
    return shapes;
}
inline JSValue snapshot() {
    auto names = JSValue::array(), kinds = record(), shapes = record();
    const auto module = nativeLTCModule();
    for (const char* name : {"LTC_Uv", "LTC_Evaluate", "LTC_Evaluate_Volume"}) {
        const auto value = module.get(name);
        push(names, name);
        kinds.set(name, value.isCallable() ? "function" : "other");
        shapes.set(name, shape(value.get("shaderNode")));
    }
    const auto uv = nativeLTCBinding("LTC_Uv").call({}, {namedInputs()});
    return record({{"names", names}, {"kinds", kinds}, {"shapes", shapes},
                   {"uvIsNode", uv.get("isNode")}, {"uvType", uv.get("type")},
                   {"uvBody", uvBody()}, {"internalShapes", internalShapes()}});
}
} // namespace LTCFixtures
