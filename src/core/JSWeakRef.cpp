#include "core/JSWeakRef.h"
namespace {
using Args = std::vector<JSValue>;
const JSValue& argument(const Args& args, std::size_t index) {
    static const JSValue undefined; return index < args.size() ? args[index] : undefined;
}
JSValue initialize(const JSValue& newTarget, const Args& args, const Args&) {
    const auto& target = argument(args, 0);
    if (!target.isObject()) throw JSTypeError("WeakRef target must be held weakly");
    static_cast<void>(JSWeakValue(target));
    const auto candidate = newTarget.get("prototype");
    return JSWeakRef::create(candidate.isObject() ? candidate : nativeWeakRefConstructor().get("prototype"), target);
}
}
JSWeakRef::JSWeakRef(const JSValue& object) {
    if (!isRef(object)) throw JSTypeError("WeakRef receiver lacks WeakRef data"); object_ = object;
}
JSValue JSWeakRef::deref() const { return object_.weakRefDeref(); }
bool JSWeakRef::isRef(const JSValue& value) { return value.hasWeakRefData(); }
JSValue JSWeakRef::create(const JSValue& prototype, const JSValue& target) {
    return JSValue::createWeakRefData(prototype, target);
}
bool isNativeWeakRef(const JSValue& value) { return JSWeakRef::isRef(value); }
JSValue nativeWeakRefConstructor() {
    static thread_local JSWeakValue cached;
    if (const auto constructor = cached.lock(); constructor && constructor->isCallable()) return *constructor;
    auto constructor = JSValue::builtinConstructor(initialize, {}, "WeakRef", 1);
    JSPropertyDescriptor descriptor;
    descriptor.value = JSValue::function(+[](const JSValue& self, const Args&, const Args&)->JSValue {
        return JSWeakRef(self).deref();
    }, {}, "deref", 0);
    descriptor.writable = descriptor.configurable = true; descriptor.enumerable = false;
    constructor.get("prototype").defineProperty("deref", descriptor);
    constructor.retainForHeapLifetime(); cached = JSWeakValue(constructor); return constructor;
}
JSValue createNativeWeakRef(const JSValue& target) {
    if (!target.isObject()) throw JSTypeError("WeakRef target must be held weakly");
    static_cast<void>(JSWeakValue(target));
    return nativeWeakRefConstructor().construct({target});
}
