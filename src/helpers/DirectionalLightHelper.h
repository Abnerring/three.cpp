#pragma once
#include "helpers/HelperUtils.h"
#include "lights/DirectionalLight.h"
class DirectionalLightHelper : public Object3D {
  public:
    std::shared_ptr<DirectionalLight> light;
    JSValue color;
    std::shared_ptr<Line> lightPlane,targetLine;
    explicit DirectionalLightHelper(std::shared_ptr<DirectionalLight> light,double size=1,HelperColor color={});
    virtual void update();
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
