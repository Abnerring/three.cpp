#include "core/UniformsGroup.h"
#include "core/EventDispatcher.h"
#include "constants.h"

namespace {
using Args = std::vector<JSValue>;
double nextId = 0;
JSValue argument(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue(); }
JSValue call(JSValue receiver, const char* key, const Args& args = {}) {
    const auto method = receiver.get(key); return method.call(receiver, args);
}
JSValue groupAdd(const JSValue& self, const Args& args, const Args&) {
    call(self.get("uniforms"), "push", {argument(args, 0)}); return self;
}
JSValue groupRemove(const JSValue& self, const Args& args, const Args&) {
    const auto index = call(self.get("uniforms"), "indexOf", {argument(args, 0)});
    if (!index.strictEquals(-1)) call(self.get("uniforms"), "splice", {index, 1});
    return self;
}
JSValue groupSetName(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver; self.set("name", argument(args, 0)); return self;
}
JSValue groupSetUsage(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver; self.set("usage", argument(args, 0)); return self;
}
JSValue groupDispose(const JSValue& self, const Args&, const Args&) {
    const auto method = self.get("dispatchEvent");
    auto event = JSValue::object(); event.set("type", "dispose"); method.call(self, {event}); return {};
}
JSValue cloneElement(const JSValue&, const Args& args, const Args&) {
    const auto uniform = argument(args, 0); return call(uniform, "clone");
}
JSValue groupCopy(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver; const auto source = argument(args, 0);
    self.set("name", source.get("name")); self.set("usage", source.get("usage"));
    const auto uniformsSource = source.get("uniforms");
    auto target = self.get("uniforms"); target.set("length", 0);
    const auto length = uniformsSource.get("length");
    for (double i = 0; i < length.toNumber(); ++i) {
        const auto uniform = uniformsSource.get(JSValue(i).toString());
        // Resolve the push receiver and method before evaluating its argument.
        const auto destination = self.get("uniforms"), push = destination.get("push");
        JSValue cloned;
        if (uniform.type() == JSValue::Type::Array) {
            const auto map = uniform.get("map");
            const auto callback = JSValue::function(cloneElement, {}, "", 1);
            cloned = map.call(uniform, {callback});
        } else cloned = call(uniform, "clone");
        push.call(destination, {cloned});
    }
    return self;
}
JSValue groupClone(const JSValue& self, const Args&, const Args&) {
    const auto constructor = self.get("constructor");
    auto created = constructor.construct(); return call(created, "copy", {self});
}
void method(JSValue object, const char* name, JSValue::NativeFunction function, std::uint32_t arity) {
    JSPropertyDescriptor descriptor; descriptor.value = JSValue::function(function, {}, name, arity);
    descriptor.writable = true; descriptor.configurable = true; object.defineProperty(name, descriptor);
}
}

JSValue UniformsGroup::constructor() {
    static thread_local auto value = [] {
        auto ctor = JSValue::constructor([](const JSValue& receiver, const Args&, const Args&) -> JSValue {
            auto self = receiver; self.set("isUniformsGroup", true);
            JSPropertyDescriptor descriptor; descriptor.value = nextId++; self.defineProperty("id", descriptor);
            self.set("name", ""); self.set("usage", StaticDrawUsage); self.set("uniforms", JSValue::array()); return {};
        }, {}, "UniformsGroup", 0);
        ctor.setPrototype(EventDispatcher::runtimePrototype().get("constructor"));
        auto p = ctor.get("prototype"); p.setPrototype(EventDispatcher::runtimePrototype());
        method(p, "add", groupAdd, 1); method(p, "remove", groupRemove, 1);
        method(p, "setName", groupSetName, 1); method(p, "setUsage", groupSetUsage, 1);
        method(p, "dispose", groupDispose, 0); method(p, "copy", groupCopy, 1); method(p, "clone", groupClone, 0);
        return ctor;
    }();
    return value;
}
JSValue UniformsGroup::prototype() { return constructor().get("prototype"); }
UniformsGroup::UniformsGroup() : object_(constructor().construct()) {}
JSValue UniformsGroup::add(JSValue uniform) { return groupAdd(object_, {uniform}, {}); }
JSValue UniformsGroup::remove(JSValue uniform) { return groupRemove(object_, {uniform}, {}); }
JSValue UniformsGroup::setName(JSValue name) { return groupSetName(object_, {name}, {}); }
JSValue UniformsGroup::setUsage(JSValue usage) { return groupSetUsage(object_, {usage}, {}); }
JSValue UniformsGroup::dispose() { return groupDispose(object_, {}, {}); }
JSValue UniformsGroup::copy(JSValue source) { return groupCopy(object_, {source}, {}); }
JSValue UniformsGroup::clone() const { return groupClone(object_, {}, {}); }
JSValue UniformsGroup::addEventListener(JSValue type, JSValue listener) { return call(object_, "addEventListener", {type, listener}); }
JSValue UniformsGroup::hasEventListener(JSValue type, JSValue listener) { return call(object_, "hasEventListener", {type, listener}); }
JSValue UniformsGroup::removeEventListener(JSValue type, JSValue listener) { return call(object_, "removeEventListener", {type, listener}); }
JSValue UniformsGroup::dispatchEvent(JSValue event) { return call(object_, "dispatchEvent", {event}); }
