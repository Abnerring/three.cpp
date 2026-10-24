#include "nodes/core/NodeFunction.h"
#include "core/JSModuleObject.h"
#include "utils.h"

JSValue nativeNodeFunctionConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "NodeFunction");
    auto constructor = JSValue::constructor([](const JSValue& receiver, const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        auto self = receiver; self.set("type", args.size() > 0 ? args[0] : JSValue{}); self.set("inputs", args.size() > 1 ? args[1] : JSValue{});
        self.set("name", args.size() > 2 && !args[2].isUndefined() ? args[2] : JSValue(""));
        self.set("precision", args.size() > 3 && !args[3].isUndefined() ? args[3] : JSValue("")); return {};
    }, {}, "NodeFunction", 2);
    auto prototype = constructor.get("prototype"); JSPropertyDescriptor method;
    method.value = JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { ThreeUtils::warn({"Abstract function."}); return {}; }, {}, "getCode", 0);
    method.writable = method.configurable = true; prototype.defineProperty("getCode", method);
    constructor.set("isNodeFunction", true); constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
NodeFunction::NodeFunction(const JSValue& type, const JSValue& inputs, const JSValue& name, const JSValue& precision) : object_(constructor().construct({type, inputs, name, precision})) {}
JSValue NodeFunction::getCode(const JSValue& name) const { return object_.get("getCode").call(object_, {name}); }
