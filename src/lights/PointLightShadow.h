#pragma once
#include "lights/LightShadow.h"
class PointLightShadow : public LightShadow {
  public:
    const bool isPointLightShadow = true;
    PointLightShadow();
    std::shared_ptr<LightShadow> clone() const override;
};
