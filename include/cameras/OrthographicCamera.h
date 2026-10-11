#pragma once
#include "cameras/Camera.h"

class OrthographicCamera : public Camera {
  public:
    const bool isOrthographicCamera = true;
    HIGH_PRECISION zoom = 1, left, right, top, bottom, near, far;
    std::optional<CameraView> view;
    OrthographicCamera(HIGH_PRECISION left = -1, HIGH_PRECISION right = 1, HIGH_PRECISION top = 1,
                       HIGH_PRECISION bottom = -1, HIGH_PRECISION near = 0.1,
                       HIGH_PRECISION far = 2000);
    OrthographicCamera& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    void setViewOffset(HIGH_PRECISION fullWidth, HIGH_PRECISION fullHeight, HIGH_PRECISION x,
                       HIGH_PRECISION y, HIGH_PRECISION width, HIGH_PRECISION height);
    void clearViewOffset();
    void updateProjectionMatrix();
    using Camera::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;

  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
