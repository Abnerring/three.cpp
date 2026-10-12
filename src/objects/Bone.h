#pragma once
#include "core/Object3D.h"

class Bone : public Object3D {
  public:
    Bone();
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
};
