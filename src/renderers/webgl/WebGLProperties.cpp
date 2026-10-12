#include "renderers/webgl/WebGLProperties.h"
#include "core/JSWeakMap.h"

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
void literalProperty(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor descriptor; descriptor.value = std::move(value);
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(name, descriptor);
}
JSValue properties(const Args& captures) { return captures[0].get("properties"); }
JSValue propertyHas(const JSValue&, const Args& args, const Args& captures) {
    const auto map = properties(captures); return map.get("has").call(map, {argument(args, 0)});
}
JSValue propertyGet(const JSValue&, const Args& args, const Args& captures) {
    const auto map = properties(captures); auto result = map.get("get").call(map, {argument(args, 0)});
    if (result.isUndefined()) {
        result = JSValue::object();
        // Upstream reads the live closure again before calling set. A get
        // override can call dispose and replace it during the preceding call.
        const auto live = properties(captures); live.get("set").call(live, {argument(args, 0), result});
    }
    return result;
}
JSValue propertyRemove(const JSValue&, const Args& args, const Args& captures) {
    const auto map = properties(captures); map.get("delete").call(map, {argument(args, 0)}); return {};
}
JSValue propertyUpdate(const JSValue&, const Args& args, const Args& captures) {
    const auto map = properties(captures); auto target = map.get("get").call(map, {argument(args, 0)});
    // EvaluatePropertyAccessWithExpressionKey rejects a nullish base before
    // ToPropertyKey. In particular a missing cache does not coerce key at all.
    if (target.isUndefined() || target.isNull()) throw JSTypeError("Cannot set properties of null or undefined");
    const auto key = argument(args, 1).toString(); target.set(key, argument(args, 2)); return {};
}
JSValue propertyDispose(const JSValue&, const Args&, const Args& captures) {
    auto state = captures[0]; state.set("properties", createNativeWeakMap()); return {};
}
}
WebGLProperties::WebGLProperties() : object_(JSValue::object()) {
    auto state = JSValue::object(nullptr); literalProperty(state, "properties", createNativeWeakMap());
    literalProperty(object_, "has", JSValue::constructor(propertyHas, {state}, "has", 1, JSValue::ConstructorKind::Callable));
    literalProperty(object_, "get", JSValue::constructor(propertyGet, {state}, "get", 1, JSValue::ConstructorKind::Callable));
    literalProperty(object_, "remove", JSValue::constructor(propertyRemove, {state}, "remove", 1, JSValue::ConstructorKind::Callable));
    literalProperty(object_, "update", JSValue::constructor(propertyUpdate, {state}, "update", 3, JSValue::ConstructorKind::Callable));
    literalProperty(object_, "dispose", JSValue::constructor(propertyDispose, {state}, "dispose", 0, JSValue::ConstructorKind::Callable));
}
JSValue WebGLProperties::has(JSValue object) { return object_.get("has").call(object_, {object}); }
JSValue WebGLProperties::get(JSValue object) { return object_.get("get").call(object_, {object}); }
JSValue WebGLProperties::remove(JSValue object) { return object_.get("remove").call(object_, {object}); }
JSValue WebGLProperties::update(JSValue object, JSValue key, JSValue value) { return object_.get("update").call(object_, {object, key, value}); }
JSValue WebGLProperties::dispose() { return object_.get("dispose").call(object_); }
