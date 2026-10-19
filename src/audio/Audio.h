#pragma once
#include "core/JSValue.h"

// Port of src/audio/Audio.js at the pinned revision, data and state machine only.
//
// Upstream builds its gain node from listener.context and wires it to listener.getInput(), so it needs a
// listener object: this port takes the same object and reads those two members, which the host provides
// alongside its context. The arithmetic of play, pause and stop is transcribed: progress accumulates as the
// elapsed context time scaled by the playback rate, a looping source wraps it by the duration, and stop resets
// it. What is not carried is spelled out in the header of each method that refuses it - the media element and
// media stream sources need DOM objects this port does not have.
class Audio {
  public:
    explicit Audio(JSValue listener = {});

    JSValue getOutput() const;
    JSValue setNodeSource(JSValue audioNode = {});
    JSValue setBuffer(JSValue audioBuffer = {});
    JSValue getLoop() const;
    JSValue setLoop(JSValue value = {});
    JSValue getLoopStart() const;
    JSValue setLoopStart(JSValue value = {});
    JSValue getLoopEnd() const;
    JSValue setLoopEnd(JSValue value = {});
    JSValue getVolume() const;
    JSValue setVolume(JSValue value = {});
    JSValue getPlaybackRate() const;
    JSValue setPlaybackRate(JSValue value = {});
    JSValue getDetune() const;
    JSValue setDetune(JSValue value = {});
    JSValue play(JSValue delay = {});
    JSValue pause();
    JSValue stop(JSValue delay = {});
    JSValue connect();
    JSValue disconnect();

    JSValue object() const { return object_; }
    JSValue& object() { return object_; }

  private:
    JSValue object_;
};