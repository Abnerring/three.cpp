#include "nodes/core/NodeAttribute.h"
#include "core/JSModuleObject.h"

JSValue nativeNodeAttributeConstructor() {
    static thread_local JSWeakValue weak;
    if (auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeAttribute constructor");
    auto constructor = JSValue::constructor([](const JSValue& receiver,
        const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        auto self = receiver;
        self.set("isNodeAttribute", true);
        self.set("name", args.size() > 0 ? args[0] : JSValue{});
        self.set("type", args.size() > 1 ? args[1] : JSValue{});
        self.set("node", args.size() > 2 && !args[2].isUndefined() ? args[2] : JSValue(nullptr));
        return {};
    }, {}, "NodeAttribute", 2);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
