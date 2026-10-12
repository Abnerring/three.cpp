#pragma once
#include "objects/Line.h"

class LineLoop : public Line {
  public:
    LineLoop(std::shared_ptr<BufferGeometry> geometry = std::make_shared<BufferGeometry>(),
             std::shared_ptr<Material> material = std::make_shared<LineBasicMaterial>());
    LineLoop(std::shared_ptr<BufferGeometry> geometry,std::vector<std::shared_ptr<Material>> materials);
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
};
