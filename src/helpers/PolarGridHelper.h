#pragma once
#include "helpers/HelperUtils.h"
#include "objects/LineSegments.h"
class PolarGridHelper : public LineSegments {
  public:
    PolarGridHelper(double radius=10,double sectors=16,double rings=8,double divisions=64,HelperColor color1=0x444444,HelperColor color2=0x888888);
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
