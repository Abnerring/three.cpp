#include "nodes/core/NodeCode.h"

#include "core/JSModuleObject.h"

JSValue nativeNodeCodeConstructor() {
    static thread_local JSWeakValue weak;
    if (auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeCode constructor");
    auto constructor = JSValue::constructor([](const JSValue& receiver,
        const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        auto self = receiver;
        self.set("name", args.size() > 0 ? args[0] : JSValue{});
        self.set("type", args.size() > 1 ? args[1] : JSValue{});
        self.set("code", args.size() > 2 && !args[2].isUndefined() ? args[2] : JSValue(""));
        JSPropertyDescriptor flag; flag.value = true;
        self.defineProperty("isNodeCode", flag);
        return {};
    }, {}, "NodeCode", 2);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
