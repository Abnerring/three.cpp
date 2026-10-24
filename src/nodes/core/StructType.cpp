#include "nodes/core/StructType.h"
#include "core/JSModuleObject.h"

JSValue nativeStructTypeConstructor() {
    static thread_local JSWeakValue weak;
    if (auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "StructType constructor");
    auto constructor = JSValue::constructor([](const JSValue& receiver,
        const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        auto self = receiver;
        self.set("name", args.size() > 0 ? args[0] : JSValue{});
        self.set("members", args.size() > 1 ? args[1] : JSValue{});
        self.set("output", false);
        return {};
    }, {}, "StructType", 2);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
