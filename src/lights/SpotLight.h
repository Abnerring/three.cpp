#pragma once
#include "lights/Light.h"
#include "lights/SpotLightShadow.h"
#include "textures/Texture.h"

class SpotLight : public Light {
  public:
    const bool isSpotLight = true;
    std::shared_ptr<Object3D> target = std::make_shared<Object3D>();
    double distance, angle, penumbra, decay;
    std::shared_ptr<Texture> map;
    std::shared_ptr<SpotLightShadow> shadow = std::make_shared<SpotLightShadow>();
    LightPower<SpotLight> power{this};
    double getPower() const;
    void setPower(double value);
    Object3D* shadowTarget() const override { return target.get(); }
    void dispose() override;
    SpotLight(const Color& color = Color(), double intensity = 1, double distance = 0, double angle = 3.14159265358979323846 / 3, double penumbra = 0, double decay = 2);
    static void registerNode(const JSValue& nodeClass);
    JSValue getLightNode() const override;
    SpotLight& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Light::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;
  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
