#include "audio/PositionalAudio.h"

#include "math/Quaternion.h"
#include "math/Vector3.h"

PositionalAudio::PositionalAudio(JSValue listener) : Audio(std::move(listener)) {
    type = "PositionalAudio";
    // Upstream asks the context for a panner, sets the HRTF model and feeds the gain with it.
    panner = context.get("createPanner").call(context, {});
    panner.set("panningModel", "HRTF");
    panner.get("connect").call(panner, {gain});
}

JSValue PositionalAudio::getOutput() const { return panner; }

Audio& PositionalAudio::connect() {
    Audio::connect();
    panner.get("connect").call(panner, {gain});
    return *this;
}

Audio& PositionalAudio::disconnect() {
    Audio::disconnect();
    panner.get("disconnect").call(panner, {gain});
    return *this;
}

double PositionalAudio::getRefDistance() const { return panner.get("refDistance").toNumber(); }

PositionalAudio& PositionalAudio::setRefDistance(double value) {
    panner.set("refDistance", JSValue(value));
    return *this;
}

double PositionalAudio::getRolloffFactor() const { return panner.get("rolloffFactor").toNumber(); }

PositionalAudio& PositionalAudio::setRolloffFactor(double value) {
    panner.set("rolloffFactor", JSValue(value));
    return *this;
}

JSValue PositionalAudio::getDistanceModel() const { return panner.get("distanceModel"); }

PositionalAudio& PositionalAudio::setDistanceModel(JSValue value) {
    panner.set("distanceModel", std::move(value));
    return *this;
}

double PositionalAudio::getMaxDistance() const { return panner.get("maxDistance").toNumber(); }

PositionalAudio& PositionalAudio::setMaxDistance(double value) {
    panner.set("maxDistance", JSValue(value));
    return *this;
}

PositionalAudio& PositionalAudio::setDirectionalCone(double coneInnerAngle, double coneOuterAngle, double coneOuterGain) {
    panner.set("coneInnerAngle", JSValue(coneInnerAngle));
    panner.set("coneOuterAngle", JSValue(coneOuterAngle));
    panner.set("coneOuterGain", JSValue(coneOuterGain));
    return *this;
}

void PositionalAudio::updateMatrixWorld(bool force) {
    Audio::updateMatrixWorld(force);
    // A source with playback control that is not playing has nothing to place yet.
    if (hasPlaybackControl && !isPlaying) return;
    Vector3 position, scale, orientation;
    Quaternion rotation;
    matrixWorld.decompose(position, rotation, scale);
    orientation.set(0.0, 0.0, 1.0).applyQuaternion(rotation);
    if (!panner.get("positionX").isUndefined()) {
        // The newer parameter path, ramped over the frame delta the listener carries.
        const double endTime = context.get("currentTime").toNumber() + listener.get("timeDelta").toNumber();
        const auto ramp = [&](const char* axis, double value) {
            auto param = panner.get(axis);
            param.get("linearRampToValueAtTime").call(param, {JSValue(value), JSValue(endTime)});
        };
        ramp("positionX", position.x);
        ramp("positionY", position.y);
        ramp("positionZ", position.z);
        ramp("orientationX", orientation.x);
        ramp("orientationY", orientation.y);
        ramp("orientationZ", orientation.z);
    } else {
        panner.get("setPosition").call(panner, {JSValue(position.x), JSValue(position.y), JSValue(position.z)});
        panner.get("setOrientation").call(panner, {JSValue(orientation.x), JSValue(orientation.y), JSValue(orientation.z)});
    }
}