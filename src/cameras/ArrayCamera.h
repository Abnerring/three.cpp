#pragma once
#include "cameras/PerspectiveCamera.h"
#include "core/SharedArray.h"

class ArrayCamera : public PerspectiveCamera {
  public:
    SharedArray<std::shared_ptr<Camera>> cameras;
    explicit ArrayCamera(SharedArray<std::shared_ptr<Camera>> array = {});
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;

  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
