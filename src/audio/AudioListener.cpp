#include "audio/AudioListener.h"

#include "audio/AudioContext.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

AudioListener::AudioListener() {
    type = "AudioListener";
    context = AudioContext::getContext();
    gain = context.get("createGain").call(context, {});
    auto destination = context.get("destination");
    gain.get("connect").call(gain, {destination});
    filter = JSValue(nullptr);
}

AudioListener& AudioListener::removeFilter() {
    // Only a routed listener has anything to undo; the direct connection is restored when it does.
    if (!filter.isNull() && !filter.isUndefined()) {
        auto current = filter;
        auto destination = context.get("destination");
        gain.get("disconnect").call(gain, {current});
        current.get("disconnect").call(current, {destination});
        gain.get("connect").call(gain, {destination});
        filter = JSValue(nullptr);
    }
    return *this;
}

AudioListener& AudioListener::setFilter(JSValue value) {
    auto destination = context.get("destination");
    if (!filter.isNull() && !filter.isUndefined()) {
        auto previous = filter;
        gain.get("disconnect").call(gain, {previous});
        previous.get("disconnect").call(previous, {destination});
    } else {
        gain.get("disconnect").call(gain, {destination});
    }
    filter = value;
    gain.get("connect").call(gain, {filter});
    filter.get("connect").call(filter, {destination});
    return *this;
}

JSValue AudioListener::getMasterVolume() const { return gain.get("gain").get("value"); }

AudioListener& AudioListener::setMasterVolume(JSValue value) {
    // Upstream ramps the master gain over ten milliseconds rather than assigning it.
    auto param = gain.get("gain");
    param.get("setTargetAtTime").call(param, {value, context.get("currentTime"), JSValue(0.01)});
    return *this;
}

void AudioListener::updateMatrixWorld(bool force) {
    Object3D::updateMatrixWorld(force);
    timer_.update();
    auto listener = context.get("listener");
    timeDelta = timer_.getDelta();
    Vector3 position, scale, forward, up;
    Quaternion rotation;
    matrixWorld.decompose(position, rotation, scale);
    // The initial forward and up directions are orthogonal, so rotating them keeps the frame valid.
    forward.set(0.0, 0.0, -1.0).applyQuaternion(rotation);
    up.set(0.0, 1.0, 0.0).applyQuaternion(rotation);
    const auto positionX = listener.get("positionX");
    if (!positionX.isUndefined()) {
        // The newer parameter path, ramped over the frame delta.
        const auto endTime = JSValue(context.get("currentTime").toNumber() + timeDelta);
        const auto ramp = [&](const char* axis, double value) {
            auto param = listener.get(axis);
            param.get("linearRampToValueAtTime").call(param, {JSValue(value), endTime});
        };
        ramp("positionX", position.x);
        ramp("positionY", position.y);
        ramp("positionZ", position.z);
        ramp("forwardX", forward.x);
        ramp("forwardY", forward.y);
        ramp("forwardZ", forward.z);
        ramp("upX", up.x);
        ramp("upY", up.y);
        ramp("upZ", up.z);
    } else {
        listener.get("setPosition").call(listener, {JSValue(position.x), JSValue(position.y), JSValue(position.z)});
        listener.get("setOrientation").call(listener, {JSValue(forward.x), JSValue(forward.y), JSValue(forward.z),
                                                       JSValue(up.x), JSValue(up.y), JSValue(up.z)});
    }
}