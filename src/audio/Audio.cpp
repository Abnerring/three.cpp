#include "audio/Audio.h"

#include <cmath>
#include <stdexcept>

Audio::Audio(JSValue listener) : listener(std::move(listener)) {
    type = "Audio";
    context = this->listener.get("context");
    if (context.isUndefined()) throw std::runtime_error("THREE.Audio: the listener carries no context");
    gain = context.get("createGain").call(context, {});
    const auto input = this->listener.get("getInput").call(this->listener, {});
    gain.get("connect").call(gain, {input});
    buffer = JSValue(nullptr);
    source = JSValue(nullptr);
}

JSValue Audio::getOutput() const { return gain; }

Audio& Audio::setNodeSource(JSValue audioNode) {
    // A node source has no playback control, which is why play and pause refuse it.
    hasPlaybackControl = false;
    sourceType = "audioNode";
    source = std::move(audioNode);
    connect();
    return *this;
}

Audio& Audio::setBuffer(JSValue audioBuffer) {
    sourceType = "buffer";
    buffer = std::move(audioBuffer);
    return *this;
}

Audio& Audio::setLoop(bool value) {
    loop = value;
    return *this;
}

Audio& Audio::setLoopStart(double value) {
    loopStart = value;
    return *this;
}

Audio& Audio::setLoopEnd(double value) {
    loopEnd = value;
    return *this;
}

double Audio::getVolume() const {
    auto param = gain.get("gain");
    return param.get("value").toNumber();
}

Audio& Audio::setVolume(double value) {
    // Upstream assigns the automation value directly, which is what the gain param exposes.
    auto param = gain.get("gain");
    param.set("value", JSValue(value));
    return *this;
}

Audio& Audio::setPlaybackRate(double value) {
    playbackRate = value;
    if (hasPlaybackControl && !source.isNull() && !source.isUndefined()) {
        auto assigned = source;
        assigned.set("playbackRate", JSValue(playbackRate));
    }
    return *this;
}

Audio& Audio::setDetune(double value) {
    detune = value;
    if (hasPlaybackControl && !source.isNull() && !source.isUndefined()) {
        auto assigned = source;
        assigned.set("detune", JSValue(detune));
    }
    return *this;
}

Audio& Audio::play(double delay) {
    if (isPlaying || !hasPlaybackControl) return *this;   // upstream warns and returns
    startedAt = context.get("currentTime").toNumber() + delay;
    auto created = context.get("createBufferSource").call(context, {});
    created.set("buffer", buffer);
    created.set("loop", JSValue(loop));
    created.set("loopStart", JSValue(loopStart));
    created.set("loopEnd", JSValue(loopEnd));
    created.get("start").call(created, {JSValue(startedAt), JSValue(progress + offset),
                                        JSValue(std::numeric_limits<double>::quiet_NaN())});
    isPlaying = true;
    source = created;
    setDetune(detune);
    setPlaybackRate(playbackRate);
    return connect();
}

Audio& Audio::pause() {
    if (!hasPlaybackControl) return *this;
    if (isPlaying) {
        const double elapsed = std::max(context.get("currentTime").toNumber() - startedAt, 0.0);
        double wrapped = progress + elapsed * playbackRate;
        if (loop) {
            // A looping source wraps its progress by the duration, falling back to the buffer duration.
            double duration = buffer.get("duration").toNumber();
            if (!(duration > 0)) duration = 0;
            if (duration > 0) wrapped = std::fmod(wrapped, duration);
        }
        progress = wrapped;
        source.get("stop").call(source, {});
        auto ended = source;
        ended.set("onended", JSValue(nullptr));
        isPlaying = false;
    }
    return *this;
}

Audio& Audio::stop(double delay) {
    if (!hasPlaybackControl) return *this;
    progress = 0;
    if (!source.isNull() && !source.isUndefined()) {
        source.get("stop").call(source, {JSValue(context.get("currentTime").toNumber() + delay)});
        auto ended = source;
        ended.set("onended", JSValue(nullptr));
    }
    isPlaying = false;
    return *this;
}

Audio& Audio::connect() {
    if (!filters.empty() && !source.isNull() && !source.isUndefined()) {
        source.get("connect").call(source, {filters.front()});
        for (std::size_t i = 1; i < filters.size(); i += 1)
            filters[i - 1].get("connect").call(filters[i - 1], {filters[i]});
        filters.back().get("connect").call(filters.back(), {getOutput()});
    } else if (!source.isNull() && !source.isUndefined()) {
        source.get("connect").call(source, {getOutput()});
    }
    return *this;
}

Audio& Audio::disconnect() {
    if (!filters.empty() && !source.isNull() && !source.isUndefined()) {
        source.get("disconnect").call(source, {});
        for (auto& filter : filters) filter.get("disconnect").call(filter, {});
    } else if (!source.isNull() && !source.isUndefined()) {
        source.get("disconnect").call(source, {});
    }
    return *this;
}

void Audio::updateMatrixWorld(bool force) { Object3D::updateMatrixWorld(force); }