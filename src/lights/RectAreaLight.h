#pragma once
#include "lights/Light.h"

class RectAreaLight : public Light {
  public:
    const bool isRectAreaLight = true;
    double width, height;
    LightPower<RectAreaLight> power{this};
    double getPower() const;
    void setPower(double value);
    RectAreaLight(const Color& color = Color(), double intensity = 1, double width = 10, double height = 10);
    static void registerNode(const JSValue& nodeClass);
    JSValue getLightNode() const override;
    RectAreaLight& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Light::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;
  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
