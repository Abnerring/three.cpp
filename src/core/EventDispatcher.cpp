#include "core/EventDispatcher.h"
#include <algorithm>

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
