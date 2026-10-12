#pragma once
#include "lights/SpotLight.h"
class IESSpotLight : public SpotLight {
  public:
    std::shared_ptr<Texture> iesMap;
    using SpotLight::SpotLight;
    static void registerNode(const JSValue& nodeClass);
    JSValue getLightNode() const override;
    IESSpotLight& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
};
