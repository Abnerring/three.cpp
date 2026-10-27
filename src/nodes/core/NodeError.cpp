#include "nodes/core/NodeError.h"
#include "core/JSError.h"
#include "core/JSModuleObject.h"

JSValue nativeNodeErrorConstructor() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "NodeError");
    auto holder = JSValue::object(nullptr); const auto base = nativeErrorConstructor();
    auto ctor = JSValue::builtinConstructor([](const JSValue& target, const std::vector<JSValue>& args, const std::vector<JSValue>& captures) -> JSValue {
        auto self = captures[0].get("constructor").getPrototype().construct({args.empty() ? JSValue{} : args[0]}, target);
        self.set("name", "NodeError"); self.set("stackTrace", args.size() > 1 && !args[1].isUndefined() ? args[1] : JSValue(nullptr)); return self;
    }, {holder}, "NodeError", 1);
    holder.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
