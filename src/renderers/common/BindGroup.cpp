#include "renderers/common/BindGroup.h"
#include "core/JSModuleObject.h"

JSValue nativeBindGroupConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BindGroup constructor");
    auto state = JSValue::object(nullptr); state.set("id", 0);
    auto constructor = JSValue::constructor([](const JSValue& receiver,
        const std::vector<JSValue>& args, const std::vector<JSValue>& captures) -> JSValue {
        // Parameter defaults are evaluated before the constructor body starts.
        const auto name = !args.empty() && !args[0].isUndefined() ? args[0] : JSValue("");
        const auto bindings = args.size() > 1 && !args[1].isUndefined() ? args[1] : JSValue::array();
        auto self = receiver;
        self.set("name", name); self.set("bindings", bindings);
        auto state = captures[0]; const auto id = state.get("id").toNumber(); state.set("id", id + 1);
        self.set("id", id);
        return {};
    }, {state}, "BindGroup", 0);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
BindGroup::BindGroup(const JSValue& name, const JSValue& bindings) {
    static_cast<void>(JSWeakValue(name)); static_cast<void>(JSWeakValue(bindings));
    object_ = constructor().construct({name, bindings});
}
JSValue BindGroup::object() const { static_cast<void>(JSWeakValue(object_)); return object_; }
JSValue BindGroup::constructor() { return nativeBindGroupConstructor(); }
JSValue BindGroup::prototype() { return constructor().get("prototype"); }
