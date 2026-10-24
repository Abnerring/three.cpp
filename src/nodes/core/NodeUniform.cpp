#include "nodes/core/NodeUniform.h"
#include "core/JSModuleObject.h"

JSValue nativeNodeUniformConstructor() {
    static thread_local JSWeakValue weak;
    if (auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeUniform constructor");
    auto constructor = JSValue::constructor([](const JSValue& receiver,
        const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        auto self = receiver;
        self.set("isNodeUniform", true);
        self.set("name", args.size() > 0 ? args[0] : JSValue{});
        self.set("type", args.size() > 1 ? args[1] : JSValue{});
        self.set("node", args.size() > 2 ? args[2] : JSValue{});
        return {};
    }, {}, "NodeUniform", 3);
    auto prototype = constructor.get("prototype");
    for (const auto* field : {"value", "id", "groupNode"}) {
        JSPropertyDescriptor property;
        property.getter = JSValue::function([](const JSValue& self,
            const std::vector<JSValue>&, const std::vector<JSValue>& captures) {
            return self.get("node").get(captures[0]);
        }, {field}, std::string("get ") + field, 0);
        property.configurable = true;
        if (std::string(field) == "value") property.setter = JSValue::function([](const JSValue& self,
            const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
            auto node = self.get("node"); node.set("value", args.empty() ? JSValue{} : args[0]); return {};
        }, {}, "set value", 1);
        prototype.defineProperty(field, property);
    }
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
