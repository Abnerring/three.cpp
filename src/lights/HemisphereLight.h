#pragma once
#include "lights/Light.h"

class HemisphereLight : public Light {
  public:
    const bool isHemisphereLight = true;
    Color groundColor;
    HemisphereLight(const Color& skyColor = Color(), const Color& groundColor = Color(), double intensity = 1);
    static void registerNode(const JSValue& nodeClass);
    JSValue getLightNode() const override;
    HemisphereLight& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Light::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;
  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
