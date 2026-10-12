#include "core/JSWeakMap.h"

namespace {
using Args = std::vector<JSValue>;
const JSValue& argument(const Args& args, std::size_t index) {
    static const JSValue undefined;
    return index < args.size() ? args[index] : undefined;
}
void install(JSValue object, const char* name, JSValue::NativeFunction callback, unsigned arity) {
    JSPropertyDescriptor descriptor;
    descriptor.value = JSValue::function(callback, {}, name, arity);
    descriptor.writable = descriptor.configurable = true; descriptor.enumerable = false;
    object.defineProperty(name, descriptor);
}
JSValue initialize(const JSValue& newTarget, const Args& args, const Args&) {
    // OrdinaryCreateFromConstructor reads prototype before iterable handling.
    const auto candidate = newTarget.get("prototype");
    const auto prototype = candidate.isObject() ? candidate : nativeWeakMapConstructor().get("prototype");
    const auto map = JSWeakMap::create(prototype);
    const auto& iterable = argument(args, 0);
    if (!iterable.isUndefined() && !iterable.isNull())
        throw JSTypeError("WeakMap iterable initialization requires runtime Symbol.iterator support");
    return map;
}
}
JSWeakMap::JSWeakMap() : object_(createNativeWeakMap()) {}
JSWeakMap::JSWeakMap(const JSValue& object) {
    if (!isMap(object)) throw JSTypeError("WeakMap receiver lacks WeakMap data");
    object_ = object;
}
bool JSWeakMap::has(const JSValue& key) const { return object_.weakMapHas(key); }
JSValue JSWeakMap::get(const JSValue& key) const { return object_.weakMapGet(key); }
bool JSWeakMap::remove(const JSValue& key) { return object_.weakMapErase(key); }
JSValue JSWeakMap::set(const JSValue& key, const JSValue& value) {
    object_.weakMapSet(key, value); return object_;
}
bool JSWeakMap::isMap(const JSValue& value) { return value.hasWeakMapData(); }
JSValue JSWeakMap::create(const JSValue& prototype) { return JSValue::createWeakMapData(prototype); }
bool isNativeWeakMap(const JSValue& value) { return JSWeakMap::isMap(value); }
JSValue nativeWeakMapConstructor() {
    // The cache does not own the heap. Within a live runtime, the global
    // constructor must retain its identity and user-modified prototype.
    static thread_local JSWeakValue cached;
    if (const auto constructor = cached.lock(); constructor && constructor->isCallable()) return *constructor;
    auto constructor = JSValue::builtinConstructor(initialize, {}, "WeakMap", 0);
    const auto prototype = constructor.get("prototype");
    install(prototype, "delete", +[](const JSValue& self, const Args& args, const Args&)->JSValue {
        return JSWeakMap(self).remove(argument(args, 0));
    }, 1);
    install(prototype, "get", +[](const JSValue& self, const Args& args, const Args&)->JSValue {
        return JSWeakMap(self).get(argument(args, 0));
    }, 1);
    install(prototype, "has", +[](const JSValue& self, const Args& args, const Args&)->JSValue {
        return JSWeakMap(self).has(argument(args, 0));
    }, 1);
    install(prototype, "set", +[](const JSValue& self, const Args& args, const Args&)->JSValue {
        return JSWeakMap(self).set(argument(args, 0), argument(args, 1));
    }, 2);
    constructor.retainForHeapLifetime();
    cached = JSWeakValue(constructor); return constructor;
}
JSValue createNativeWeakMap() { return nativeWeakMapConstructor().construct(); }
