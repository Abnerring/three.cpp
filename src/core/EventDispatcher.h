#pragma once

#include "core/JsonValue.h"
#include "core/JSValue.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class EventDispatcher;
class Object3D;

struct Event {
    std::string type;
    EventDispatcher* target = nullptr;
    Object3D* child = nullptr;
    JsonValue data;
};

class EventDispatcher {
  public:
    using Listener = std::shared_ptr<std::function<void(Event&)>>;
    virtual ~EventDispatcher() = default;
    // Runtime-backed classes share this prototype and keep listener/event
    // identities in the managed object graph.
    static JSValue runtimePrototype();
    static JSValue runtimeObject();
    Listener addEventListener(const std::string& type, std::function<void(Event&)> callback);
    void addEventListener(const std::string& type, const Listener& listener);
    bool hasEventListener(const std::string& type, const Listener& listener) const;
    void removeEventListener(const std::string& type, const Listener& listener);
    void dispatchEvent(Event& event);
    void dispatchEvent(Event&& event) { dispatchEvent(event); }

  private:
    std::unordered_map<std::string, std::vector<Listener>> listeners_;
};
