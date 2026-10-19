#include "audio/AudioContext.h"

#include <stdexcept>

namespace {
JSValue& contextSlot() {
    static thread_local JSValue value;
    return value;
}
}

JSValue AudioContext::getContext() {
    auto& context = contextSlot();
    if (context.isUndefined())
        // Upstream constructs one from the browser; here the host has to install it.
        throw std::runtime_error("THREE.AudioContext: no context installed; call AudioContext::setContext");
    return context;
}

void AudioContext::setContext(JSValue value) { contextSlot() = std::move(value); }