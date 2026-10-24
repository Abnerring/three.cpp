#include "nodes/core/NodeCache.h"
#include "core/JSModuleObject.h"
#include "core/JSWeakMap.h"

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) {
    return index < args.size() ? args[index] : JSValue{};
}
void validate(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
}
JSValue call(const JSValue& receiver, const char* name, const Args& args = {}) {
    return receiver.get(name).call(receiver, args);
}
JSValue initialize(const JSValue& self, const Args& args, const Args& captures) {
    auto state = captures[0];
    auto receiver = self;
    const auto id = state.get("id").toNumber();
    // The postfix increment takes effect before the observable id assignment.
    state.set("id", id + 1);
    receiver.set("id", id);
    receiver.set("nodesData", createNativeWeakMap());
    const auto parent = argument(args, 0);
    receiver.set("parent", parent.isUndefined() ? JSValue(nullptr) : parent);
    return {};
}
JSValue getData(const JSValue& self, const Args& args, const Args&) {
    const auto node = argument(args, 0);
    auto data = call(self.get("nodesData"), "get", {node});
    if (data.isUndefined() && !self.get("parent").isNull()) {
        // The source reads parent again for the call. A getter may return a
        // different object on its second invocation.
        data = call(self.get("parent"), "getData", {node});
    }
    return data;
}
JSValue setData(const JSValue& self, const Args& args, const Args&) {
    call(self.get("nodesData"), "set", {argument(args, 0), argument(args, 1)});
    return {};
}
void method(JSValue prototype, const char* name, JSValue::NativeFunction callback,
            unsigned arity) {
    JSPropertyDescriptor descriptor;
    descriptor.value = JSValue::function(callback, {}, name, arity);
    descriptor.writable = descriptor.configurable = true;
    prototype.defineProperty(name, descriptor);
}
} // namespace

JSValue nativeNodeCacheConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto constructor = weak.lock(); constructor && constructor->isConstructible())
        return *constructor;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeCache constructor");
    auto state = JSValue::object(nullptr);
    JSPropertyDescriptor id;
    id.value = 0;
    id.writable = true;
    state.defineProperty("id", id);
    auto constructor = JSValue::constructor(initialize, {state}, "NodeCache", 0);
    auto prototype = constructor.get("prototype");
    method(prototype, "getData", getData, 1);
    method(prototype, "setData", setData, 2);
    constructor.retainForHeapLifetime();
    weak = JSWeakValue(constructor);
    return constructor;
}
NodeCache::NodeCache() : object_(constructor().construct()) {}
NodeCache::NodeCache(const JSValue& parent) {
    validate(parent);
    object_ = constructor().construct({parent});
}
NodeCache::NodeCache(const std::shared_ptr<NodeCache>& parent)
    : NodeCache(parent ? parent->object() : JSValue(nullptr)) {}
JSValue NodeCache::constructor() {
    return nativeNodeCacheConstructor();
}
JSValue NodeCache::prototype() {
    return constructor().get("prototype");
}
JSValue NodeCache::object() const {
    validate(object_);
    return object_;
}
JSValue NodeCache::id() const {
    return object().get("id");
}
JSValue NodeCache::parent() const {
    return object().get("parent");
}
JSValue NodeCache::getData(const JSValue& node) const {
    validate(object_);
    validate(node);
    return call(object_, "getData", {node});
}
JSValue NodeCache::setData(const JSValue& node, const JSValue& data) {
    validate(object_);
    validate(node);
    validate(data);
    return call(object_, "setData", {node, data});
}
