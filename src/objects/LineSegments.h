#pragma once
#include "objects/Line.h"

class LineSegments : public Line {
  public:
    LineSegments(std::shared_ptr<BufferGeometry> geometry = std::make_shared<BufferGeometry>(),
                 std::shared_ptr<Material> material = std::make_shared<LineBasicMaterial>());
    LineSegments(std::shared_ptr<BufferGeometry> geometry,std::vector<std::shared_ptr<Material>> materials);
    LineSegments& computeLineDistances() override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
};
