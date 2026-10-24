#include "nodes/core/NodeFrame.h"
#include "core/JSModuleObject.h"
#include "core/JSWeakMap.h"
#include <chrono>

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) {
    return index < args.size() ? args[index] : JSValue{};
}
JSValue defaultPerformance() {
    auto performance = JSValue::object();
    performance.set("now", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
        return std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now().time_since_epoch()).count();
    }, {}, "now", 0));
    return performance;
}
JSValue host() {
    static thread_local JSWeakValue cached;
    if (const auto found = cached.lock(); found && found->isObject()) return *found;
    auto value = JSValue::object();
    value.set("performance", defaultPerformance());
    value.retainForHeapLifetime();
    cached = JSWeakValue(value);
    return value;
}
JSValue now() {
    const auto performance = host().get("performance");
    const auto method = performance.get("now");
    return method.call(performance);
}
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive();
    const auto b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    return a.toNumber() + b.toNumber();
}
void initialize(JSValue self) {
    self.set("time", 0);
    self.set("deltaTime", 0);
    self.set("frameId", 0);
    self.set("renderId", 0);
    self.set("updateMap", createNativeWeakMap());
    self.set("updateBeforeMap", createNativeWeakMap());
    self.set("updateAfterMap", createNativeWeakMap());
    for (const char* field : {"renderer", "material", "camera", "object", "scene", "compute"})
        self.set(field, nullptr);
}
JSValue getMaps(JSValue referenceMap, const JSValue& reference) {
    const auto get = referenceMap.get("get");
    auto maps = get.call(referenceMap, {reference});
    if (maps.isUndefined()) {
        maps = JSValue::object();
        // Object-literal entries create own data properties. Inherited setters
        // and nonwritable properties must not intercept their initialization.
        JSPropertyDescriptor field;
        field.value = 0;
        field.enumerable = field.writable = field.configurable = true;
        maps.defineProperty("renderId", field);
        maps.defineProperty("frameId", field);
        const auto set = referenceMap.get("set");
        set.call(referenceMap, {reference, maps});
    }
    return maps;
}
void updateNode(JSValue self, const JSValue& node, const std::string& name) {
    const bool before = name == "updateBeforeNode";
    const bool after = name == "updateAfterNode";
    const char* getType = before ? "getUpdateBeforeType" : after ? "getUpdateAfterType" : "getUpdateType";
    const char* callback = before ? "updateBefore" : after ? "updateAfter" : "update";
    const char* mapName = before ? "updateBeforeMap" : after ? "updateAfterMap" : "updateMap";
    const auto typeMethod = node.get(getType);
    const auto type = typeMethod.call(node, {self});
    const auto referenceMethod = node.get("updateReference");
    const auto reference = referenceMethod.call(node, {self});
    if (type.strictEquals("frame") || type.strictEquals("render")) {
        const char* field = type.strictEquals("frame") ? "frameId" : "renderId";
        const auto method = self.get("_getMaps");
        const auto referenceMap = self.get(mapName);
        auto maps = method.call(self, {referenceMap, reference});
        const auto savedId = maps.get(field);
        const auto currentId = self.get(field);
        if (!savedId.strictEquals(currentId)) {
            if (before) {
                const auto previous = maps.get(field);
                maps.set(field, self.get(field));
                const auto update = node.get(callback);
                if (update.call(node, {self}).strictEquals(false)) maps.set(field, previous);
            } else {
                const auto update = node.get(callback);
                if (!update.call(node, {self}).strictEquals(false)) maps.set(field, self.get(field));
            }
        }
    } else if (type.strictEquals("object")) {
        const auto update = node.get(callback);
        update.call(node, {self});
    }
}
JSValue dispatch(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    const auto& name = captures[0].string();
    if (name == "_getMaps") return getMaps(argument(args, 0), argument(args, 1));
    if (name != "update") {
        updateNode(self, argument(args, 0), name);
        return {};
    }
    self.set("frameId", self.get("frameId").toNumber() + 1);
    if (self.get("lastTime").isUndefined()) self.set("lastTime", now());
    // The first clock call precedes lastTime's getter; numeric coercion is later.
    const auto current = now();
    const auto previous = self.get("lastTime");
    const double currentNumber = current.toNumber();
    const double previousNumber = previous.toNumber();
    self.set("deltaTime", (currentNumber - previousNumber) / 1000);
    self.set("lastTime", now());
    const auto time = self.get("time");
    const auto delta = self.get("deltaTime");
    self.set("time", add(time, delta));
    return {};
}
}  // namespace

JSValue NodeFrame::prototype() {
    static thread_local JSWeakValue cached;
    if (const auto found = cached.lock(); found && found->isObject()) return *found;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeFrame prototype");
    auto constructor = JSValue::constructor([](const JSValue& self, const Args&, const Args&) -> JSValue {
        initialize(self);
        return {};
    }, {}, "NodeFrame", 0);
    auto prototype = constructor.get("prototype");
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
             {"_getMaps", 2}, {"updateBeforeNode", 1}, {"updateAfterNode", 1}, {"updateNode", 1}, {"update", 0}}) {
        JSPropertyDescriptor descriptor;
        descriptor.configurable = descriptor.writable = true;
        descriptor.value = JSValue::function(dispatch, {name}, name, arity);
        prototype.defineProperty(name, descriptor);
    }
    constructor.retainForHeapLifetime();
    cached = JSWeakValue(prototype);
    return prototype;
}
NodeFrame::NodeFrame() : object_(JSValue::object(prototype())) { initialize(object_); }
void NodeFrame::setClock(JSValue performance) { host().set("performance", performance); }
void NodeFrame::resetClock() { host().set("performance", defaultPerformance()); }
void NodeFrame::update() { object_.get("update").call(object_); }
void NodeFrame::updateNode(JSValue node) { object_.get("updateNode").call(object_, {node}); }
void NodeFrame::updateBeforeNode(JSValue node) { object_.get("updateBeforeNode").call(object_, {node}); }
void NodeFrame::updateAfterNode(JSValue node) { object_.get("updateAfterNode").call(object_, {node}); }
