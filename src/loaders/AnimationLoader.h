#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"

// Port of src/loaders/AnimationLoader.js at the pinned revision (see upstream.json).
//
// The whole of parse is a loop over the serialized clips, each handed to AnimationClip.parse and
// collected into one array; load fetches text through FileLoader and runs the same parse, sending a
// failure to onError and then to manager.itemError in the order the pinned source uses. Upstream
// logs to the console when no onError is supplied and there is no console natively, so that branch
// is recorded rather than reproduced.
class AnimationLoader : public Loader {
  public:
    explicit AnimationLoader(JSValue manager={});
    static AnimationLoader fromObject(JSValue value){return AnimationLoader(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue parse(JSValue json);
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    AnimationLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};