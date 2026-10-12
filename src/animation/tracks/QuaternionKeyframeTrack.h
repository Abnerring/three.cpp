#pragma once
#include "animation/KeyframeTrack.h"

class QuaternionKeyframeTrack : public KeyframeTrack {
  public:
    QuaternionKeyframeTrack(JSValue name = {}, Array times = {}, Array values = {}, JSValue interpolation = {});
    std::shared_ptr<Interpolant> InterpolantFactoryMethodLinear(Array result = {}) override;
  protected:
    std::shared_ptr<KeyframeTrack> constructClone(JSValue name, Array times, Array values) const override;
};
