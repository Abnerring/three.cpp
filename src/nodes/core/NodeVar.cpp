#include "nodes/core/NodeVar.h"

#include "core/JSModuleObject.h"

JSValue nativeNodeVarConstructor() {
    static thread_local JSWeakValue weak;
    if (auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeVar constructor");
    auto constructor = JSValue::constructor([](const JSValue& receiver,
        const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        auto self = receiver;
        const auto arg = [&](std::size_t i, JSValue fallback = {}) {
            return i < args.size() && !args[i].isUndefined() ? args[i] : fallback;
        };
        self.set("isNodeVar", true);
        self.set("name", arg(0));
        self.set("type", arg(1));
        self.set("readOnly", arg(2, false));
        self.set("local", arg(4, false));
        self.set("count", arg(3, nullptr));
        return {};
    }, {}, "NodeVar", 2);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
