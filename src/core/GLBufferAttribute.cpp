// Direct port of three.js GLBufferAttribute.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See repository THREEJS-LICENSE.
#include "core/GLBufferAttribute.h"

namespace {
using Args = std::vector<JSValue>;
const JSValue& argument(const Args& args, std::size_t index) {
    static const JSValue undefined;
    return index < args.size() ? args[index] : undefined;
}
void validate(const JSValue& value) { static_cast<void>(JSWeakValue(value)); }
JSValue initialize(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    // These are assignments, not CreateDataProperty: inherited setters and
    // nonwritable properties participate in the same order as the JS class.
    self.set("isGLBufferAttribute", true);
    self.set("name", "");
    self.set("buffer", argument(args, 0));
    self.set("type", argument(args, 1));
    self.set("itemSize", argument(args, 2));
    self.set("elementSize", argument(args, 3));
    self.set("count", argument(args, 4));
    self.set("normalized", argument(args, 5).isUndefined() ? JSValue(false) : argument(args, 5));
    self.set("version", 0);
    return {};
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    const auto& name = captures[0].string();
    if (name == "needsUpdate") {
        if (argument(args, 0).strictEquals(true)) {
            const double next = self.get("version").toNumber() + 1;
            self.set("version", next);
        }
        return {};
    }
    if (name == "setBuffer") self.set("buffer", argument(args, 0));
    else if (name == "setType") {
        self.set("type", argument(args, 0));
        self.set("elementSize", argument(args, 1));
    } else if (name == "setItemSize") self.set("itemSize", argument(args, 0));
    else self.set("count", argument(args, 0));
    return self;
}
JSValue call(const JSValue& self, const char* name, const Args& args) {
    validate(self); for (const auto& value : args) validate(value);
    return self.get(name).call(self, args);
}
}

JSValue nativeGLBufferAttributeConstructor() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isConstructible()) return *value;
    auto constructor = JSValue::constructor(initialize, {}, "GLBufferAttribute", 5);
    auto prototype = constructor.get("prototype");
    JSPropertyDescriptor setter;
    setter.setter = JSValue::function(method, {"needsUpdate"}, "set needsUpdate", 1);
    setter.configurable = true;
    prototype.defineProperty("needsUpdate", setter);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
             {"setBuffer", 1}, {"setType", 2}, {"setItemSize", 1}, {"setCount", 1}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(method, {name}, name, arity);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }
    constructor.retainForHeapLifetime();
    cached = JSWeakValue(constructor);
    return constructor;
}
GLBufferAttribute::GLBufferAttribute(const JSValue& buffer, const JSValue& type, const JSValue& itemSize,
                                   const JSValue& elementSize, const JSValue& count, const JSValue& normalized) {
    for (const auto* value : {&buffer, &type, &itemSize, &elementSize, &count, &normalized}) validate(*value);
    object_ = constructor().construct({buffer, type, itemSize, elementSize, count, normalized});
}
GLBufferAttribute::GLBufferAttribute(const JSValue& object, Existing) { validate(object); object_ = object; }
GLBufferAttribute::GLBufferAttribute(const GLBufferAttribute& other) { validate(other.object_); object_=other.object_; }
GLBufferAttribute& GLBufferAttribute::operator=(const GLBufferAttribute& other) {
    validate(object_); validate(other.object_); object_=other.object_; return *this;
}
JSValue GLBufferAttribute::constructor() { return nativeGLBufferAttributeConstructor(); }
JSValue GLBufferAttribute::prototype() { return constructor().get("prototype"); }
JSValue GLBufferAttribute::object() const { validate(object_); return object_; }
JSValue GLBufferAttribute::get(const std::string& key) const { validate(object_); return object_.get(key); }
void GLBufferAttribute::setProperty(const std::string& key, const JSValue& value) { validate(object_); validate(value); object_.set(key, value); }
JSValue GLBufferAttribute::setBuffer(const JSValue& value) { validate(value); return call(object_, "setBuffer", {value}); }
JSValue GLBufferAttribute::setType(const JSValue& value, const JSValue& size) {
    validate(value); validate(size); return call(object_, "setType", {value, size});
}
JSValue GLBufferAttribute::setItemSize(const JSValue& value) { validate(value); return call(object_, "setItemSize", {value}); }
JSValue GLBufferAttribute::setCount(const JSValue& value) { validate(value); return call(object_, "setCount", {value}); }
JSValue nativeGLBufferAttribute(const GLBufferAttribute& attribute) { return attribute.object(); }
