#include "audio/Audio.h"

#include <cmath>
#include <stdexcept>

namespace {
JSValue get(const JSValue& self, const char* name) { return self.get(name); }
}

Audio::Audio(JSValue listener) {
    object_ = JSValue::object();
    object_.set("type", "Audio");
    object_.set("listener", listener);
    const auto context = listener.get("context");
    if (context.isUndefined()) throw std::runtime_error("THREE.Audio: the listener carries no context");
    object_.set("context", context);
    const auto gain = context.get("createGain").call(context, {});
    const auto input = listener.get("getInput").call(listener, {});
    gain.get("connect").call(gain, {input});
    object_.set("gain", gain);
    object_.set("autoplay", false);
    object_.set("buffer", JSValue(nullptr));
    object_.set("detune", 0);
    object_.set("loop", false);
    object_.set("loopStart", 0);
    object_.set("loopEnd", 0);
    object_.set("offset", 0);
    object_.set("playbackRate", 1);
    object_.set("isPlaying", false);
    object_.set("hasPlaybackControl", true);
    object_.set("sourceType", "empty");
    object_.set("source", JSValue(nullptr));
    object_.set("filters", JSValue::array());
    object_.set("_progress", 0);
    object_.set("_startedAt", 0);
    object_.set("_pausedAt", 0);
}

JSValue Audio::getOutput() const { return get(object_, "gain"); }

JSValue Audio::setNodeSource(JSValue audioNode) {
    // A node source has no playback control, which is why play and pause refuse it below.
    object_.set("hasPlaybackControl", false);
    object_.set("sourceType", "audioNode");
    object_.set("source", audioNode);
    connect();
    return object_;
}

JSValue Audio::setBuffer(JSValue audioBuffer) {
    object_.set("sourceType", "buffer");
    object_.set("buffer", audioBuffer);
    return object_;
}

JSValue Audio::getLoop() const { return get(object_, "loop"); }
JSValue Audio::setLoop(JSValue value) { object_.set("loop", value); return object_; }
JSValue Audio::getLoopStart() const { return get(object_, "loopStart"); }
JSValue Audio::setLoopStart(JSValue value) { object_.set("loopStart", value); return object_; }
JSValue Audio::getLoopEnd() const { return get(object_, "loopEnd"); }
JSValue Audio::setLoopEnd(JSValue value) { object_.set("loopEnd", value); return object_; }

JSValue Audio::getVolume() const { return get(object_, "gain").get("gain").get("value"); }

JSValue Audio::setVolume(JSValue value) {
    // Upstream assigns the automation value directly, which is what the gain param exposes.
    auto gain = get(object_, "gain");
    auto param = gain.get("gain");
    param.set("value", value);
    return object_;
}

JSValue Audio::getPlaybackRate() const { return get(object_, "playbackRate"); }

JSValue Audio::setPlaybackRate(JSValue value) {
    object_.set("playbackRate", value);
    const auto source = get(object_, "source");
    if (get(object_, "hasPlaybackControl").toBoolean() && !source.isUndefined() && !source.isNull()) {
        auto assigned = source;
        assigned.set("playbackRate", get(object_, "playbackRate"));
    }
    return object_;
}

JSValue Audio::getDetune() const { return get(object_, "detune"); }

JSValue Audio::setDetune(JSValue value) {
    object_.set("detune", value);
    const auto source = get(object_, "source");
    if (get(object_, "hasPlaybackControl").toBoolean() && !source.isUndefined() && !source.isNull()) {
        auto assigned = source;
        assigned.set("detune", get(object_, "detune"));
    }
    return object_;
}

JSValue Audio::play(JSValue delay) {
    if (get(object_, "isPlaying").toBoolean() || !get(object_, "hasPlaybackControl").toBoolean())
        return object_;   // upstream warns and returns; the refusal is the behaviour that matters
    // The delay parameter defaults to zero upstream, which undefined means for a JS default.
    const double delayValue = delay.isUndefined() ? 0.0 : delay.toNumber();
    const auto context = get(object_, "context");
    const auto startedAt = context.get("currentTime").toNumber() + delayValue;
    object_.set("_startedAt", startedAt);
    const auto source = context.get("createBufferSource").call(context, {});
    auto configured = source;
    configured.set("buffer", get(object_, "buffer"));
    configured.set("loop", get(object_, "loop"));
    configured.set("loopStart", get(object_, "loopStart"));
    configured.set("loopEnd", get(object_, "loopEnd"));
    const double offset = get(object_, "_progress").toNumber() + get(object_, "offset").toNumber();
    source.get("start").call(source, {JSValue(startedAt), JSValue(offset), get(object_, "duration")});
    object_.set("isPlaying", true);
    object_.set("source", source);
    setDetune(get(object_, "detune"));
    setPlaybackRate(get(object_, "playbackRate"));
    return connect();
}

JSValue Audio::pause() {
    if (!get(object_, "hasPlaybackControl").toBoolean()) return object_;
    if (get(object_, "isPlaying").toBoolean()) {
        const auto context = get(object_, "context");
        const double elapsed = std::max(context.get("currentTime").toNumber() - get(object_, "_startedAt").toNumber(), 0.0);
        double progress = get(object_, "_progress").toNumber() + elapsed * get(object_, "playbackRate").toNumber();
        if (get(object_, "loop").toBoolean()) {
            // A looping source wraps its progress by the duration, falling back to the buffer duration.
            double duration = get(object_, "duration").toNumber();
            if (!(duration > 0)) duration = get(object_, "buffer").get("duration").toNumber();
            if (duration > 0) progress = std::fmod(progress, duration);
        }
        object_.set("_progress", progress);
        const auto source = get(object_, "source");
        source.get("stop").call(source, {});
        auto ended = source;
        ended.set("onended", JSValue(nullptr));
        object_.set("isPlaying", false);
    }
    return object_;
}

JSValue Audio::stop(JSValue delay) {
    if (!get(object_, "hasPlaybackControl").toBoolean()) return object_;
    object_.set("_progress", 0);
    const auto source = get(object_, "source");
    if (!source.isUndefined() && !source.isNull()) {
        const auto context = get(object_, "context");
        const double delayValue = delay.isUndefined() ? 0.0 : delay.toNumber();
        source.get("stop").call(source, {JSValue(context.get("currentTime").toNumber() + delayValue)});
        auto ended = source;
        ended.set("onended", JSValue(nullptr));
    }
    object_.set("isPlaying", false);
    return object_;
}

JSValue Audio::connect() {
    const auto filters = get(object_, "filters");
    const auto length = filters.length();
    const auto source = get(object_, "source");
    if (length > 0 && !source.isUndefined() && !source.isNull()) {
        source.get("connect").call(source, {filters.get(0)});
        for (double i = 1; i < length; i += 1)
            filters.get(i - 1).get("connect").call(filters.get(i - 1), {filters.get(i)});
        filters.get(length - 1).get("connect").call(filters.get(length - 1), {getOutput()});
    } else if (!source.isUndefined() && !source.isNull()) {
        source.get("connect").call(source, {getOutput()});
    }
    return object_;
}

JSValue Audio::disconnect() {
    const auto filters = get(object_, "filters");
    const auto length = filters.length();
    const auto source = get(object_, "source");
    if (length > 0 && !source.isUndefined() && !source.isNull()) {
        source.get("disconnect").call(source, {});
        for (double i = 0; i < length; i += 1) filters.get(i).get("disconnect").call(filters.get(i), {});
    } else if (!source.isUndefined() && !source.isNull()) {
        source.get("disconnect").call(source, {});
    }
    return object_;
}