#include "core/JSWeakMap.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args = std::vector<JSValue>; using O = JsonValue::Object; using A = JsonValue::Array;
JSValue noop(const JSValue&, const Args&, const Args&) { return {}; }
JSValue key(unsigned kind) { return kind == 0 ? JSValue::object() : kind == 1 ? JSValue::array() : JSValue::function(noop); }
Args values() { return {{}, nullptr, false, true, 0, -0.0, 1.5, std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), "", "value", JSValue::object(), JSValue::array(), JSValue::function(noop)}; }
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "undefined"; if (value.isNull()) return nullptr;
    if (value.type() == JSValue::Type::String) return value.string();
    if (value.type() == JSValue::Type::Boolean) return value.toBoolean();
    if (value.type() == JSValue::Type::Number) { const auto number = value.number();
        if (std::isnan(number)) return "NaN"; if (std::isinf(number)) return number < 0 ? "-Infinity" : "Infinity";
        return number == 0 && std::signbit(number) ? JsonValue("-0") : JsonValue(number); }
    return value.isCallable() ? "function" : value.type() == JSValue::Type::Array ? "array" : "object";
}
template<class F> JsonValue invoke(F&& function) { try { return encode(function()); } catch (const JSTypeError&) { return O{{"error", "TypeError"}}; } }
JSValue call(const JSValue& map, const char* name, const Args& args = {}) { return map.get(name).call(map, args); }
void emit(JsonValue value) { std::cout << value.dump() << '\n'; }
}
int main() {
    for (unsigned kind = 0; kind < 3; ++kind) for (unsigned index = 0; index < 15; ++index) {
        const auto map = createNativeWeakMap(), object = key(kind); const auto value = values()[index];
        const auto initial = call(map, "has", {object}), missing = call(map, "get", {object});
        const auto set = call(map, "set", {object, value}), present = call(map, "has", {object}), get = call(map, "get", {object});
        const auto same = get.sameValue(value); const auto unrelated = call(map, "get", {key(kind)});
        const auto removed = call(map, "delete", {object}), again = call(map, "delete", {object}), after = call(map, "has", {object});
        emit(O{{"kind", "identity"}, {"key", kind}, {"value", index}, {"initial", encode(initial)}, {"missing", encode(missing)},
            {"returnedSelf", set.sameValue(map)}, {"present", encode(present)}, {"get", encode(get)}, {"same", same},
            {"unrelated", encode(unrelated)}, {"removed", encode(removed)}, {"again", encode(again)}, {"after", encode(after)}});
    }
    for (unsigned index = 0; index < 12; ++index) {
        const auto map = createNativeWeakMap(), object = values()[index];
        emit(O{{"kind", "primitive"}, {"index", index}, {"has", invoke([&] { return call(map, "has", {object}); })},
            {"get", invoke([&] { return call(map, "get", {object}); })}, {"delete", invoke([&] { return call(map, "delete", {object}); })},
            {"set", invoke([&] { return call(map, "set", {object, 1}); })}});
    }
    const auto constructor = nativeWeakMapConstructor(), prototype = constructor.get("prototype");
    for (const auto* name : {"get", "has", "delete", "set"}) for (unsigned index = 0; index < 15; ++index) {
        const auto receiver = values()[index]; emit(O{{"kind", "brand"}, {"method", name}, {"index", index},
            {"result", invoke([&] { return prototype.get(name).call(receiver, {nullptr, 1}); })}});
    }
    for (const auto* name : {"get", "has", "delete", "set"}) {
        const auto method = prototype.get(name); const auto descriptor = prototype.getOwnPropertyDescriptor(name);
        emit(O{{"kind", "descriptor"}, {"method", name}, {"name", encode(method.get("name"))}, {"length", encode(method.get("length"))},
            {"writable", *descriptor->writable}, {"enumerable", *descriptor->enumerable}, {"configurable", *descriptor->configurable}});
    }
    emit(O{{"kind", "constructor"}, {"name", encode(constructor.get("name"))}, {"length", encode(constructor.get("length"))},
        {"call", invoke([&] { return constructor.call(); })}, {"null", isNativeWeakMap(constructor.construct({nullptr}))},
        {"undefined", isNativeWeakMap(constructor.construct({JSValue{}}))}, {"prototypeIsMap", isNativeWeakMap(prototype)}});
    auto newTarget = JSValue::constructor(noop, {}, "NewTarget", 0, JSValue::ConstructorKind::Callable);
    newTarget.set("prototype", 17); const auto fallback = constructor.construct({}, newTarget);
    auto custom = JSValue::object(); newTarget.set("prototype", custom); const auto derived = constructor.construct({}, newTarget);
    emit(O{{"kind", "newTarget"}, {"fallback", fallback.getPrototype().sameValue(prototype)}, {"custom", derived.getPrototype().sameValue(custom)},
        {"brand", isNativeWeakMap(derived)}, {"missing", encode(JSWeakMap(derived).get(JSValue::object()))}});
    auto map = createNativeWeakMap(), object = JSValue::object(); auto events = JSValue::array();
    object.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
        captures[0].get("push").call(captures[0], {"coerced"}); throw JSRangeError("key");
    }, {events}));
    call(map, "set", {object, object}); emit(O{{"kind", "noCoercion"}, {"same", call(map, "get", {object}).sameValue(object)},
        {"has", encode(call(map, "has", {object}))}, {"delete", encode(call(map, "delete", {object}))}, {"conversions", events.length()}});
}
