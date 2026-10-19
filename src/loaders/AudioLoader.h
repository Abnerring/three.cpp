#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"

// Port of src/loaders/AudioLoader.js at the pinned revision.
//
// The bytes come through the file loader with the array buffer response type, and the decode goes through the
// context that AudioContext hands out, which is already a host supplied object: no new interface is needed,
// only the rule that the buffer is copied first because decodeAudioData detaches it. The decode url carries a
// #decode suffix so the loading manager does not finish before the decode does, and the error path reports
// against the original url, both as upstream does.
//
// The browser decodes asynchronously while an injected context can answer inside the call, so this loader is
// synchronous like the rest of the port.
class AudioLoader : public Loader {
  public:
    explicit AudioLoader(JSValue manager = {});
    JSValue load(JSValue url = {}, JSValue onLoad = {}, JSValue onProgress = {}, JSValue onError = {}) override;
};