#pragma once
#include "helpers/HelperUtils.h"
#include "lights/SpotLight.h"
#include "objects/LineSegments.h"
class SpotLightHelper : public Object3D {
  public:
    std::shared_ptr<SpotLight> light;
    JSValue color;
    std::shared_ptr<LineSegments> cone;
    explicit SpotLightHelper(std::shared_ptr<SpotLight> light,HelperColor color={});
    virtual void update();
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
