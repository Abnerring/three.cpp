#include "core/Uniform.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
JSValue uniformConstructor();
JSValue cloneUniform(const JSValue& receiver, const Args&, const Args&) {
    const auto clone = receiver.get("value").get("clone");
    JSValue value;
    if (clone.isUndefined()) value = receiver.get("value");
    else {
        const auto current = receiver.get("value"), method = current.get("clone");
        value = method.call(current);
    }
    return Uniform(value).object();
}
JSValue uniformConstructor() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Uniform constructor");
    auto value = [] {
        auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args&) -> JSValue {
            auto self = receiver; self.set("value", args.empty() ? JSValue() : args[0]); return {};
        }, {}, "Uniform", 1);
        auto p = ctor.get("prototype");
        JSPropertyDescriptor clone; clone.value = JSValue::function(cloneUniform, {}, "clone", 0);
        clone.writable = true; clone.configurable = true; p.defineProperty("clone", clone);
        return ctor;
    }();
    value.retainForHeapLifetime();
    cached = JSWeakValue(value);
    return value;
}
}
JSValue Uniform::prototype() { return uniformConstructor().get("prototype"); }
Uniform::Uniform(JSValue value) : object_(uniformConstructor().construct({value})) {}
Uniform Uniform::clone() const { return fromObject(cloneUniform(object_, {}, {})); }
