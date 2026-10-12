#pragma once
#include "lights/Light.h"
#include "math/SphericalHarmonics3.h"

class LightProbe : public Light {
  public:
    const bool isLightProbe = true;
    std::shared_ptr<SphericalHarmonics3> sh;
    LightProbe(std::shared_ptr<SphericalHarmonics3> sh = std::make_shared<SphericalHarmonics3>(), double intensity = 1);
    static void registerNode(const JSValue& nodeClass);
    JSValue getLightNode() const override;
    LightProbe& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Light::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;
  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
