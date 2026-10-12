#pragma once
#include "helpers/HelperUtils.h"
#include "objects/LineSegments.h"
class AxesHelper : public LineSegments {
  public:
    explicit AxesHelper(double size=1);
    AxesHelper& setColors(HelperColor xAxisColor,HelperColor yAxisColor,HelperColor zAxisColor);
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
