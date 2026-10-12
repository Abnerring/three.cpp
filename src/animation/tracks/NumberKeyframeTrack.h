#pragma once
#include "animation/KeyframeTrack.h"

class NumberKeyframeTrack : public KeyframeTrack {
  public:
    NumberKeyframeTrack(JSValue name = {}, Array times = {}, Array values = {}, JSValue interpolation = {});
  protected:
    std::shared_ptr<KeyframeTrack> constructClone(JSValue name, Array times, Array values) const override;
};
