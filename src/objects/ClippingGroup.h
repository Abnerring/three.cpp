#pragma once
#include "objects/Group.h"
#include "core/SharedArray.h"
#include "math/Plane.h"

class ClippingGroup : public Group {
  public:
    SharedArray<std::shared_ptr<Plane>> clippingPlanes;
    JSValue enabled = true, clipIntersection = false, clipShadows = false;
    ClippingGroup();
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;

  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
