#pragma once
#include "core/Object3D.h"
#include "core/Timer.h"

// Port of src/audio/AudioListener.js at the pinned revision.
//
// The listener owns the master gain: it is built from the shared audio context, connected to the context
// destination, and optionally routed through one filter. Its world transform is pushed into the context
// listener every frame, with both the modern parameter ramp and the older setPosition/setOrientation call
// kept, exactly as upstream branches on them.
class AudioListener : public Object3D {
  public:
    AudioListener();

    JSValue getInput() const { return gain; }
    AudioListener& removeFilter();
    JSValue getFilter() const { return filter; }
    AudioListener& setFilter(JSValue value = {});
    JSValue getMasterVolume() const;
    AudioListener& setMasterVolume(JSValue value = {});
    void updateMatrixWorld(bool force = false) override;

    JSValue context, gain, filter;
    double timeDelta = 0;

  private:
    Timer timer_;
};