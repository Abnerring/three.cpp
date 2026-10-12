#pragma once
#include "lights/Light.h"
#include "lights/DirectionalLightShadow.h"

class DirectionalLight : public Light {
  public:
    const bool isDirectionalLight = true;
    std::shared_ptr<Object3D> target = std::make_shared<Object3D>();
    std::shared_ptr<DirectionalLightShadow> shadow = std::make_shared<DirectionalLightShadow>();
    Object3D* shadowTarget() const override { return target.get(); }
    void dispose() override;
    DirectionalLight(const Color& color = Color(), double intensity = 1);
    static void registerNode(const JSValue& nodeClass);
    JSValue getLightNode() const override;
    DirectionalLight& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Light::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;
  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
