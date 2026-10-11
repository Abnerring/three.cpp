#pragma once
#include "core/Object3D.h"

struct CameraView {
    bool enabled = true;
    HIGH_PRECISION fullWidth = 1, fullHeight = 1;
    HIGH_PRECISION offsetX = 0, offsetY = 0, width = 1, height = 1;
    bool operator==(const CameraView&) const = default;
    JsonValue toJSON() const;
};

class Camera : public Object3D {
  public:
    Matrix4 matrixWorldInverse, projectionMatrix, projectionMatrixInverse;
    int coordinateSystem = 2000; // WebGLCoordinateSystem; Vulkan uses 2001 depth range.
    bool reversedDepth = false;
    Camera();
    Camera& copy(const Object3D& source, bool recursive = true) override;
    Vector3& getWorldDirection(Vector3& target) override;
    void updateMatrixWorld(bool force = false) override;
    void updateWorldMatrix(bool updateParents = false, bool updateChildren = false,
                           bool force = false) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;

  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;

  private:
    void updateViewMatrix();
};
