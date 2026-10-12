#pragma once
#include "animation/KeyframeTrack.h"

class StringKeyframeTrack : public KeyframeTrack {
  public:
    StringKeyframeTrack(JSValue name = {}, Array times = {}, Array values = {}, JSValue interpolation = {});
  protected:
    std::shared_ptr<KeyframeTrack> constructClone(JSValue name, Array times, Array values) const override;
};
