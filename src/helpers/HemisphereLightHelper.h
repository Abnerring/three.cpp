#pragma once
#include "helpers/HelperUtils.h"
#include "lights/HemisphereLight.h"
class HemisphereLightHelper : public Object3D {
  public:
    std::shared_ptr<HemisphereLight> light;
    JSValue color;
    std::shared_ptr<MeshBasicMaterial> material;
    explicit HemisphereLightHelper(std::shared_ptr<HemisphereLight> light,double size=1,HelperColor color={});
    virtual void update();
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
