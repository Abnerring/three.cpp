#pragma once
#include "audio/Audio.h"

// Port of src/audio/PositionalAudio.js at the pinned revision.
//
// Everything positional lives on a panner node created from the same context and connected to the gain, and
// the public setters are the panner parameters under the upstream names. The output is the panner rather than
// the gain, which is what makes the graph positional.
//
// updateMatrixWorld pushes the world transform into the panner every frame, but only while the source is
// playing or has no playback control, and takes either the parameter ramp or the older set position and
// orientation call. The frame delta comes from the listener object the audio was built with, which is where a
// JS AudioListener keeps it.
class PositionalAudio : public Audio {
  public:
    explicit PositionalAudio(JSValue listener = {});

    JSValue getOutput() const override;
    Audio& connect() override;
    Audio& disconnect() override;
    double getRefDistance() const;
    PositionalAudio& setRefDistance(double value);
    double getRolloffFactor() const;
    PositionalAudio& setRolloffFactor(double value);
    JSValue getDistanceModel() const;
    PositionalAudio& setDistanceModel(JSValue value = {});
    double getMaxDistance() const;
    PositionalAudio& setMaxDistance(double value);
    PositionalAudio& setDirectionalCone(double coneInnerAngle, double coneOuterAngle, double coneOuterGain);
    void updateMatrixWorld(bool force = false) override;

    JSValue panner;
};