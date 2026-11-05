#pragma once
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSOperators.h"
#include "core/JSModuleObject.h"
#include <cmath>
#include <string_view>

// Helpers for statically translated geometry algorithms. Source is compiled
// into C++ statements; these helpers do not interpret source or an AST.
namespace GeometryRuntime {
using Args = std::vector<JSValue>;
inline JSValue arg(const Args& a, std::size_t i) { return i < a.size() ? a[i] : JSValue{}; }
inline void own(JSValue object, const JSValue& key, JSValue value) {
    JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
inline JSValue scope(JSValue parent = nullptr) {
    auto e = JSValue::object(nullptr); e.setInternalSlot("parent", parent); return e;
}
inline void declare(JSValue env, const std::string& name, bool mutableBinding) {
    auto cell = JSValue::object(nullptr);
    cell.setInternalSlot("initialized", false); cell.setInternalSlot("mutable", mutableBinding);
    env.setInternalSlot("binding:" + name, cell);
}
inline JSValue cell(JSValue env, const std::string& name) {
    while (!env.isNull()) {
        const auto c = env.getInternalSlot("binding:" + name);
        if (!c.isUndefined()) return c;
        env = env.getInternalSlot("parent");
    }
    return {};
}
inline void initialize(JSValue env, const std::string& name, JSValue value) {
    auto c = cell(env, name);
    if (c.isUndefined() || c.getInternalSlot("initialized").toBoolean())
        throw std::logic_error("Invalid geometry lexical initialization: " + name);
    c.setInternalSlot("value", value); c.setInternalSlot("initialized", true);
}
inline JSValue read(JSValue env, const std::string& name) {
    const auto c = cell(env, name);
    if (c.isUndefined()) return nativeGlobalBinding(name);
    if (!c.getInternalSlot("initialized").toBoolean())
        throw JSReferenceError("Cannot access lexical binding before initialization");
    return c.getInternalSlot("value");
}
inline JSValue write(JSValue env, const std::string& name, JSValue value) {
    auto c = cell(env, name);
    if (c.isUndefined()) throw JSReferenceError("Unbound geometry lexical assignment: " + name);
    if (!c.getInternalSlot("initialized").toBoolean())
        throw JSReferenceError("Cannot access lexical binding before initialization");
    if (!c.getInternalSlot("mutable").toBoolean()) throw JSTypeError("Assignment to constant variable");
    c.setInternalSlot("value", value); return value;
}
inline JSValue setReference(JSValue object, JSValue key, JSValue value) {
    if (object.isNull() || object.isUndefined()) throw JSTypeError("Cannot assign property of null/undefined");
    const auto property = key.toPropertyKey();
    if (object.isObject()) object.set(property, value);
    else if (!object.toObject().reflectSet(property, value, object)) throw JSTypeError("Cannot assign primitive property");
    return value;
}
inline JSValue deleteReference(JSValue object, JSValue key) {
    if (object.isNull() || object.isUndefined()) throw JSTypeError("Cannot delete property of null/undefined");
    const auto property = key.toPropertyKey();
    if (!object.toObject().erase(property)) throw JSTypeError("Cannot delete nonconfigurable property");
    return true;
}
inline JSValue binary(std::string_view op, JSValue left, JSValue right) {
    if (op == "===") return left.strictEquals(right);
    if (op == "!==") return !left.strictEquals(right);
    if (op == "in") return right.has(left);
    if (op == "+") {
        const auto a = left.toPrimitive(), b = right.toPrimitive();
        if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String) {
            const auto x = a.toString(), y = b.toString(); return x + y;
        }
        const auto x = a.toNumber(), y = b.toNumber(); return x + y;
    }
    if (op == "<") return JSOperators::less(left, right);
    if (op == ">") return JSOperators::greater(left, right);
    if (op == "<=") return JSOperators::lessEqual(left, right);
    if (op == ">=") return JSOperators::greaterEqual(left, right);
    const auto a = left.toNumber(), b = right.toNumber();
    if (op == "-") return a - b;
    if (op == "*") return a * b;
    if (op == "/") return a / b;
    if (op == "%") return std::fmod(a, b);
    throw std::logic_error("Unsupported translated geometry operator");
}
inline void spread(Args& result, JSValue iterable) {
    auto iterator = getJSIterator(iterable);
    while (const auto value = stepJSIterator(iterator)) result.push_back(*value);
}
inline void copyProperties(JSValue target, JSValue source) {
    if (source.isNull() || source.isUndefined()) return;
    if (!source.isObject()) source = source.toObject();
    for (const auto& key : source.ownPropertyKeys()) {
        const auto d = source.getOwnPropertyDescriptor(key);
        if (d && d->enumerable.value_or(false)) own(target, key, source.get(key));
    }
}
inline void bindImport(JSValue env, const std::string& name, JSValue value) {
    declare(env, name, false); initialize(env, name, value);
}
}
