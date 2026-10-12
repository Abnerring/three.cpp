#pragma once
#include "lights/LightShadow.h"
class SpotLightShadow : public LightShadow {
  public:
    const bool isSpotLightShadow = true;
    double focus = 1, aspect = 1;
    SpotLightShadow();
    void updateMatrices(Light& light) override;
    SpotLightShadow& copy(const LightShadow& source) override;
    std::shared_ptr<LightShadow> clone() const override;
    JsonValue toJSON() const override;
};
