#pragma once
#include "loaders/Loader.h"
#include "animation/AnimationClip.h"

class AnimationLoader : public Loader {
  public:
    explicit AnimationLoader(JSValue manager={});
    static JSValue prototype();
    using Loader::parse;
    AnimationClip::Clips parseAnimations(JSValue json);
    AnimationClip::Clips parse(const JsonValue& json){return parseAnimations(JSValue::fromJSON(json));}
};
