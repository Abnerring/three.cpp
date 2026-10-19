#pragma once
#include "core/JSValue.h"
#include "core/Object3D.h"

// Port of src/audio/Audio.js at the pinned revision.
//
// Upstream extends Object3D, so this does too, and its state lives in native members rather than in a JS
// wrapper: the listener, the context, the gain node, the current source and the filters are JSValues because
// the host supplies them, everything else is a plain field. The arithmetic of play, pause and stop is
// transcribed: progress accumulates as the elapsed context time scaled by the playback rate, a looping source
// wraps it by the duration, and stop resets it. The media element and media stream sources are absent by
// intent, because they need DOM objects this port does not have.
class Audio : public Object3D {
  public:
    explicit Audio(JSValue listener = {});

    virtual JSValue getOutput() const;
    virtual Audio& setNodeSource(JSValue audioNode = {});
    virtual Audio& setBuffer(JSValue audioBuffer = {});
    bool getLoop() const { return loop; }
    Audio& setLoop(bool value);
    double getLoopStart() const { return loopStart; }
    Audio& setLoopStart(double value);
    double getLoopEnd() const { return loopEnd; }
    Audio& setLoopEnd(double value);
    double getVolume() const;
    Audio& setVolume(double value);
    double getPlaybackRate() const { return playbackRate; }
    Audio& setPlaybackRate(double value);
    double getDetune() const { return detune; }
    Audio& setDetune(double value);
    Audio& play(double delay = 0.0);
    Audio& pause();
    Audio& stop(double delay = 0.0);
    virtual Audio& connect();
    virtual Audio& disconnect();
    void updateMatrixWorld(bool force = false) override;

    JSValue listener, context, gain, buffer, source;
    std::vector<JSValue> filters;
    std::string sourceType = "empty";
    bool autoplay = false, loop = false, isPlaying = false, hasPlaybackControl = true;
    double detune = 0, loopStart = 0, loopEnd = 0, offset = 0, playbackRate = 1;
    double progress = 0, startedAt = 0, pausedAt = 0;
};