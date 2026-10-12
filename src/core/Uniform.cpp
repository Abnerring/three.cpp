#include "core/Uniform.h"

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
    static thread_local auto value = [] {
        auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args&) -> JSValue {
            auto self = receiver; self.set("value", args.empty() ? JSValue() : args[0]); return {};
        }, {}, "Uniform", 1);
        auto p = ctor.get("prototype");
        JSPropertyDescriptor clone; clone.value = JSValue::function(cloneUniform, {}, "clone", 0);
        clone.writable = true; clone.configurable = true; p.defineProperty("clone", clone);
        return ctor;
    }();
    return value;
}
}
JSValue Uniform::prototype() { return uniformConstructor().get("prototype"); }
Uniform::Uniform(JSValue value) : object_(uniformConstructor().construct({value})) {}
Uniform Uniform::clone() const { return fromObject(cloneUniform(object_, {}, {})); }
