#pragma once
#include "animation/AnimationClip.h"
#include "math/Color.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Native port of examples/jsm/animation/AnimationClipCreator.js from the pinned three.js
// revision (see upstream.json). The class exposes six static factories that build small
// demonstration clips.
//
// `CreateShakeAnimation` and `CreatePulsationAnimation` read Math.random; the native port keeps
// the same rule with an injectable source, and an empty std::function selects the native
// default (the deterministic MathUtils::seededRandom()), because there is no Math.random here.
class AnimationClipCreator {
  public:
    using RandomSource = std::function<double()>;

    static void setRandomGenerator(RandomSource randomFunction);

    static std::shared_ptr<AnimationClip> CreateRotationAnimation(double period, const std::string& axis = "x");
    static std::shared_ptr<AnimationClip> CreateScaleAxisAnimation(double period, const std::string& axis = "x");
    static std::shared_ptr<AnimationClip> CreateShakeAnimation(double duration, double shakeScale);
    static std::shared_ptr<AnimationClip> CreatePulsationAnimation(double duration, double pulseScale);
    static std::shared_ptr<AnimationClip> CreateVisibilityAnimation(double duration);
    static std::shared_ptr<AnimationClip> CreateMaterialColorAnimation(double duration, const std::vector<Color>& colors);
};