#pragma once
#include "lights/Light.h"
#include "lights/PointLightShadow.h"

class PointLight : public Light {
  public:
    const bool isPointLight = true;
    double distance, decay;
    std::shared_ptr<PointLightShadow> shadow = std::make_shared<PointLightShadow>();
    LightPower<PointLight> power{this};
    double getPower() const;
    void setPower(double value);
    void dispose() override;
    PointLight(const Color& color = Color(), double intensity = 1, double distance = 0, double decay = 2);
    static void registerNode(const JSValue& nodeClass);
    JSValue getLightNode() const override;
    PointLight& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Light::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;
  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
