#pragma once
#include "lights/SpotLight.h"
class ProjectorLight : public SpotLight {
  public:
    std::optional<double> aspect;
    using SpotLight::SpotLight;
    static void registerNode(const JSValue& nodeClass);
    JSValue getLightNode() const override;
    ProjectorLight& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
};
