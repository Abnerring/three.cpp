#pragma once
#include "helpers/HelperUtils.h"
#include "objects/LineSegments.h"
class SkeletonHelper : public LineSegments {
  public:
    std::shared_ptr<Object3D> root;
    SharedArray<std::shared_ptr<Object3D>> bones;
    explicit SkeletonHelper(std::shared_ptr<Object3D> object);
    void updateMatrixWorld(bool force=false) override;
    SkeletonHelper& setColors(const Color& color1,const Color& color2);
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
  private:
    SkeletonHelper(std::shared_ptr<Object3D> object,std::vector<std::shared_ptr<Object3D>> bones);
};
