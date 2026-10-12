#pragma once
#include "helpers/HelperUtils.h"
#include "math/Plane.h"
class PlaneHelper : public Line {
  public:
    std::shared_ptr<Plane> plane;
    double size;
    explicit PlaneHelper(std::shared_ptr<Plane> plane=nullptr,double size=1,HelperColor color=0xffff00);
    void updateMatrixWorld(bool force=false) override;
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
