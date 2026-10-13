#include "core/JSFinalizationRegistry.h"
namespace {
using Args = std::vector<JSValue>;
const JSValue& argument(const Args& args, std::size_t index) {
    static const JSValue undefined; return index < args.size() ? args[index] : undefined;
}
void install(JSValue prototype, const char* name, JSValue::NativeFunction callback, unsigned arity) {
    JSPropertyDescriptor descriptor; descriptor.value = JSValue::function(callback, {}, name, arity);
    descriptor.writable = descriptor.configurable = true; descriptor.enumerable = false;
    prototype.defineProperty(name, descriptor);
}
JSValue initialize(const JSValue& newTarget, const Args& args, const Args&) {
    const auto& callback = argument(args, 0);
    if (!callback.isCallable()) throw JSTypeError("FinalizationRegistry cleanup must be callable");
    static_cast<void>(JSWeakValue(callback));
    const auto candidate = newTarget.get("prototype");
    return JSFinalizationRegistry::create(candidate.isObject() ? candidate : nativeFinalizationRegistryConstructor().get("prototype"), callback);
}
}
JSFinalizationRegistry::JSFinalizationRegistry(const JSValue& object) {
    if (!isRegistry(object)) throw JSTypeError("FinalizationRegistry receiver lacks registry data"); object_ = object;
}
void JSFinalizationRegistry::registerTarget(const JSValue& target, const JSValue& heldValue, const JSValue& token) {
    object_.finalizationRegistryRegister(target, heldValue, token);
}
bool JSFinalizationRegistry::unregister(const JSValue& token) { return object_.finalizationRegistryUnregister(token); }
bool JSFinalizationRegistry::isRegistry(const JSValue& value) { return value.hasFinalizationRegistryData(); }
JSValue JSFinalizationRegistry::create(const JSValue& prototype, const JSValue& callback) {
    return JSValue::createFinalizationRegistryData(prototype, callback);
}
void JSFinalizationRegistry::cleanup(const JSValue& registry) { auto root = registry; root.finalizationRegistryCleanup(); }
bool isNativeFinalizationRegistry(const JSValue& value) { return JSFinalizationRegistry::isRegistry(value); }
JSValue nativeFinalizationRegistryConstructor() {
    static thread_local JSWeakValue cached;
    if (const auto constructor = cached.lock(); constructor && constructor->isCallable()) return *constructor;
    auto constructor = JSValue::builtinConstructor(initialize, {}, "FinalizationRegistry", 1);
    const auto prototype = constructor.get("prototype");
    install(prototype, "register", +[](const JSValue& self, const Args& args, const Args&)->JSValue {
        JSFinalizationRegistry(self).registerTarget(argument(args, 0), argument(args, 1), argument(args, 2)); return {};
    }, 2);
    install(prototype, "unregister", +[](const JSValue& self, const Args& args, const Args&)->JSValue {
        return JSFinalizationRegistry(self).unregister(argument(args, 0));
    }, 1);
    constructor.retainForHeapLifetime(); cached = JSWeakValue(constructor); return constructor;
}
JSValue createNativeFinalizationRegistry(const JSValue& callback) {
    if (!callback.isCallable()) throw JSTypeError("FinalizationRegistry cleanup must be callable");
    static_cast<void>(JSWeakValue(callback));
    return nativeFinalizationRegistryConstructor().construct({callback});
}
