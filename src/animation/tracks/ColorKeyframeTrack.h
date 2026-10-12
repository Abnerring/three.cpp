#pragma once
#include "animation/KeyframeTrack.h"

class ColorKeyframeTrack : public KeyframeTrack {
  public:
    ColorKeyframeTrack(JSValue name = {}, Array times = {}, Array values = {}, JSValue interpolation = {});
  protected:
    std::shared_ptr<KeyframeTrack> constructClone(JSValue name, Array times, Array values) const override;
};
