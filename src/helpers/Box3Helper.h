#pragma once
#include "helpers/HelperUtils.h"
#include "objects/LineSegments.h"
class Box3Helper : public LineSegments {
  public:
    std::shared_ptr<Box3> box;
    explicit Box3Helper(std::shared_ptr<Box3> box=nullptr,HelperColor color=0xffff00);
    void updateMatrixWorld(bool force=false) override;
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
