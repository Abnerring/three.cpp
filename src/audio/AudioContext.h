#pragma once
#include "core/JSValue.h"

// Port of src/audio/AudioContext.js at the pinned revision.
//
// Upstream keeps one module level context and builds it lazily from window.AudioContext. There is no window
// here, so the lazy construction is the host boundary: the host installs a context through setContext, and
// getContext refuses - naming that call - when nothing has been installed. Everything else is the upstream
// behaviour: one shared value, returned as is, replaceable at any time.
class AudioContext {
  public:
    static JSValue getContext();
    static void setContext(JSValue value);
};