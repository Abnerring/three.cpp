#pragma once
#include "scenes/Fog.h"

class FogExp2 : public FogBase {
  public:
    const bool isFogExp2 = true;
    std::string name;
    Color color;
    HIGH_PRECISION density;
    FogExp2(const Color& color, HIGH_PRECISION density = 0.00025)
        : color(color), density(density) {}
    FogExp2(int color, HIGH_PRECISION density = 0.00025) : FogExp2(Color(color), density) {}
    FogExp2(const std::string& color, HIGH_PRECISION density = 0.00025)
        : FogExp2(Color(color), density) {}
    std::shared_ptr<FogBase> clone() const override {
        return std::make_shared<FogExp2>(color, density);
    }
    JsonValue toJSON() const override {
        return JsonValue::Object{
            {"type", "FogExp2"}, {"name", name}, {"color", color.getHex()}, {"density", density}};
    }
};
