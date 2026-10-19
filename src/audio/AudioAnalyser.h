#pragma once
#include "core/JSValue.h"

// Port of src/audio/AudioAnalyser.js at the pinned revision.
//
// The analyser node comes from the audio context the given Audio carries, its fft size is set before the data
// array is sized from the frequency bin count, and the audio output is connected to it - all in that order,
// because the bin count is only final once the size is set. The byte frequency data is read into the array the
// constructor allocated, and the average is the plain mean of those bytes.
class AudioAnalyser {
  public:
    explicit AudioAnalyser(JSValue audio = {}, double fftSize = 2048);

    JSValue getFrequencyData();
    double getAverageFrequency();

    JSValue object() const { return object_; }
    JSValue& object() { return object_; }

  private:
    JSValue object_;
};