#include "nodes/core/NodeFunctionInput.h"
#include "core/JSModuleObject.h"

JSValue nativeNodeFunctionInputConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "NodeFunctionInput");
    auto constructor = JSValue::constructor([](const JSValue& receiver, const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        const auto arg = [&](std::size_t i, const JSValue& fallback = {}) { return i < args.size() && !args[i].isUndefined() ? args[i] : fallback; };
        auto self = receiver; self.set("type", arg(0)); self.set("name", arg(1)); self.set("count", arg(2, nullptr)); self.set("qualifier", arg(3, "")); self.set("isConst", arg(4, false)); return {};
    }, {}, "NodeFunctionInput", 2);
    constructor.set("isNodeFunctionInput", true); constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
NodeFunctionInput::NodeFunctionInput(const JSValue& type, const JSValue& name, const JSValue& count, const JSValue& qualifier, const JSValue& isConst)
    : object_(constructor().construct({type, name, count, qualifier, isConst})) {}
