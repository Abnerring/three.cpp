#pragma once
#include "nodes/core/Node.h"
#include "nodes/core/constants.h"
#include "core/JSIterator.h"
#include "core/JSModuleObject.h"
#include "core/JSStringEncoding.h"
#include <cmath>
#include <limits>

namespace UtilityNodeRuntime {
using Args = std::vector<JSValue>;
inline JSValue arg(const Args& args, std::size_t i, const JSValue& fallback = {}) {
    return i < args.size() && !args[i].isUndefined() ? args[i] : fallback;
}
struct Invocation {
    JSValue object, method;
    JSValue operator()(const Args& args = {}) const { return method.call(object, args); }
};
inline Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
inline JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String) {
        const auto x = a.toString(); return x + b.toString();
    }
    const auto x = a.toNumber(); return x + b.toNumber();
}
inline bool less(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(JSValue::PreferredType::Number), b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String) return jsUTF16(a.string()) < jsUTF16(b.string());
    const auto x = a.toNumber(); return x < b.toNumber();
}
inline bool greater(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(JSValue::PreferredType::Number), b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String) return jsUTF16(a.string()) > jsUTF16(b.string());
    const auto x = a.toNumber(); return x > b.toNumber();
}
inline bool greaterEqual(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(JSValue::PreferredType::Number), b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String) return jsUTF16(a.string()) >= jsUTF16(b.string());
    const auto x = a.toNumber(); return x >= b.toNumber();
}
template<class F> void each(const JSValue& iterable, F body) {
    auto iterator = getJSIterator(iterable);
    while (const auto value = stepJSIterator(iterator)) {
        try { body(*value); } catch (...) { closeJSIterator(iterator, std::current_exception()); }
    }
}
inline void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; object.defineProperty(key,d);
}
inline JSValue makeConstructor(const char* name, unsigned length, JSValue::NativeFunction initialize,
                               std::initializer_list<std::pair<const char*, unsigned>> methods,
                               JSValue::NativeFunction method, const Args& extraCaptures = {}) {
    const auto base = nativeNodeConstructor(); auto holder = JSValue::object(nullptr);
    auto constructor = JSValue::builtinConstructor(initialize, {holder}, name, length);
    holder.set("constructor", constructor); constructor.setPrototype(base);
    auto prototype = constructor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c)->JSValue { return c[0]; }, {name}, "get type", 0);
    type.configurable = true; constructor.defineProperty("type",type);
    for (const auto& [key, arity] : methods) {
        Args captures{key,prototype}; captures.insert(captures.end(),extraCaptures.begin(),extraCaptures.end());
        JSPropertyDescriptor d; d.value = JSValue::function(method,captures,key,arity); d.writable=d.configurable=true; prototype.defineProperty(key,d);
    }
    constructor.retainForHeapLifetime(); return constructor;
}
inline JSValue superConstruct(const JSValue& newTarget, const Args& captures, const Args& args = {}) {
    return captures[0].get("constructor").getPrototype().construct(args,newTarget);
}
inline void superMethod(const JSValue& self, const char* name, const Args& captures, const Args& args) {
    captures[1].getPrototype().get(name,self).call(self,args);
}
}
