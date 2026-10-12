#pragma once
#include "helpers/HelperUtils.h"
#include "lights/PointLight.h"
class PointLightHelper : public Mesh {
  public:
    std::shared_ptr<PointLight> light;
    JSValue color;
    explicit PointLightHelper(std::shared_ptr<PointLight> light,double sphereSize=1,HelperColor color={});
    virtual void update();
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
