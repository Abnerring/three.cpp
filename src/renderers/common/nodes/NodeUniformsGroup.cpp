#include "renderers/common/nodes/NodeUniformsGroup.h"
#include "renderers/common/UniformsGroup.h"
#include "core/JSModuleObject.h"

JSValue nativeNodeUniformsGroupConstructor() {
    using Args = std::vector<JSValue>;
    static thread_local JSWeakValue weak; static thread_local bool initializing = false;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    JSModuleInitializationScope scope(initializing, "NodeUniformsGroup");
    auto state = JSValue::object(nullptr); state.set("nextId", 0); const auto base = nativeUniformsGroupConstructor();
    auto ctor = JSValue::builtinConstructor([](const JSValue& target, const Args& args, const Args& c) -> JSValue {
        const auto state = c[0]; const auto name = args.empty() ? JSValue{} : args[0];
        auto self = state.get("constructor").getPrototype().construct({name}, target);
        auto mutableState = state; const auto id = state.get("nextId").number(); mutableState.set("nextId", id + 1);
        self.set("id", id); self.set("groupNode", args.size() > 1 ? args[1] : JSValue{}); self.set("isNodeUniformsGroup", true); return self;
    }, {state}, "NodeUniformsGroup", 2);
    state.set("constructor", ctor); ctor.setPrototype(base); ctor.get("prototype").setPrototype(base.get("prototype"));
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
