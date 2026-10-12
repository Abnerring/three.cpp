#pragma once
#include "helpers/HelperUtils.h"
#include "objects/LineSegments.h"
class GridHelper : public LineSegments {
  public:
    GridHelper(double size=10,double divisions=10,HelperColor color1=0x444444,HelperColor color2=0x888888);
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
