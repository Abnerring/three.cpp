#include "core/EventDispatcher.h"
#include <algorithm>

namespace {
using Args = std::vector<JSValue>;
JSValue eventArg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue(); }
JSValue eventCall(JSValue object, const char* name, const Args& args = {}) {
    const auto method = object.get(name); return method.call(object, args);
}
JSValue eventMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string();
    const auto type = eventArg(args, 0), listener = eventArg(args, 1);
    if (name == "addEventListener") {
        if (self.get("_listeners").isUndefined()) self.set("_listeners", JSValue::object());
        auto listeners = self.get("_listeners");
        if (listeners.get(type.toString()).isUndefined()) listeners.set(type.toString(), JSValue::array());
        auto array = listeners.get(type.toString());
        if (eventCall(array, "indexOf", {listener}).strictEquals(-1)) {
            array = listeners.get(type.toString()); eventCall(array, "push", {listener});
        }
        return {};
    }
    const auto listeners = self.get("_listeners");
    if (listeners.isUndefined()) return name == "hasEventListener" ? JSValue(false) : JSValue();
    if (name == "hasEventListener") {
        if (listeners.get(type.toString()).isUndefined()) return false;
        return !eventCall(listeners.get(type.toString()), "indexOf", {listener}).strictEquals(-1);
    }
    if (name == "removeEventListener") {
        const auto array = listeners.get(type.toString());
        if (!array.isUndefined()) {
            const auto index = eventCall(array, "indexOf", {listener});
            if (!index.strictEquals(-1)) eventCall(array, "splice", {index, 1});
        }
        return {};
    }
    auto event = eventArg(args, 0);
    const auto eventType = event.get("type");
    const auto listenersForType = listeners.get(eventType.toString());
    if (!listenersForType.isUndefined()) {
        event.set("target", self);
        const auto array = eventCall(listenersForType, "slice", {0});
        const auto length = array.get("length");
        for (double i = 0; i < length.toNumber(); ++i) {
            const auto callback = array.get(JSValue(i).toString());
            eventCall(callback, "call", {self, event});
        }
        event.set("target", nullptr);
    }
    return {};
}
}

JSValue EventDispatcher::runtimePrototype() {
    static thread_local JSValue prototype = [] {
        auto object = JSValue::object();
        auto constructor = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
            throw JSTypeError("Class constructor EventDispatcher cannot be invoked without 'new'");
        }, {}, "EventDispatcher", 0);
        JSPropertyDescriptor prototypeProperty;
        prototypeProperty.value = object;
        prototypeProperty.writable = false; prototypeProperty.enumerable = false; prototypeProperty.configurable = false;
        constructor.defineProperty("prototype", prototypeProperty);
        JSPropertyDescriptor constructorProperty;
        constructorProperty.value = constructor;
        constructorProperty.writable = true; constructorProperty.enumerable = false; constructorProperty.configurable = true;
        object.defineProperty("constructor", constructorProperty);
        for (const auto* name : {"addEventListener", "hasEventListener", "removeEventListener", "dispatchEvent"}) {
            JSPropertyDescriptor descriptor;
            descriptor.value = JSValue::function(eventMethod, {name}, name, std::string(name) == "dispatchEvent" ? 1 : 2);
            descriptor.writable = true; descriptor.enumerable = false; descriptor.configurable = true;
            object.defineProperty(name, descriptor);
        }
        return object;
    }();
    return prototype;
}
JSValue EventDispatcher::runtimeObject() { return JSValue::object(runtimePrototype()); }

EventDispatcher::Listener EventDispatcher::addEventListener(const std::string& type,
                                                            std::function<void(Event&)> callback) {
    auto listener = std::make_shared<std::function<void(Event&)>>(std::move(callback));
    addEventListener(type, listener);
    return listener;
}

void EventDispatcher::addEventListener(const std::string& type, const Listener& listener) {
    if (!listener || hasEventListener(type, listener))
        return;
    listeners_[type].push_back(listener);
}

bool EventDispatcher::hasEventListener(const std::string& type, const Listener& listener) const {
    const auto found = listeners_.find(type);
    return found != listeners_.end() &&
           std::find(found->second.begin(), found->second.end(), listener) != found->second.end();
}

void EventDispatcher::removeEventListener(const std::string& type, const Listener& listener) {
    const auto found = listeners_.find(type);
    if (found == listeners_.end())
        return;
    auto& array = found->second;
    const auto entry = std::find(array.begin(), array.end(), listener);
    if (entry != array.end())
        array.erase(entry);
}

void EventDispatcher::dispatchEvent(Event& event) {
    const auto found = listeners_.find(event.type);
    if (found == listeners_.end())
        return;
    event.target = this;
    const auto array = found->second;
    for (const auto& listener : array)
        (*listener)(event);
    event.target = nullptr;
}
