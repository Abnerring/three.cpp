#pragma once
#include "cameras/Camera.h"
#include "math/Vector2.h"

class PerspectiveCamera : public Camera {
  public:
    const bool isPerspectiveCamera = true;
    HIGH_PRECISION fov, zoom = 1, near, far, focus = 10, aspect, filmGauge = 35, filmOffset = 0;
    std::optional<CameraView> view;
    PerspectiveCamera(HIGH_PRECISION fov = 50, HIGH_PRECISION aspect = 1, HIGH_PRECISION near = 0.1,
                      HIGH_PRECISION far = 2000);
    PerspectiveCamera& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    void setFocalLength(HIGH_PRECISION focalLength);
    HIGH_PRECISION getFocalLength() const;
    HIGH_PRECISION getEffectiveFOV() const;
    HIGH_PRECISION getFilmWidth() const;
    HIGH_PRECISION getFilmHeight() const;
    void getViewBounds(HIGH_PRECISION distance, Vector2& minTarget, Vector2& maxTarget) const;
    Vector2& getViewSize(HIGH_PRECISION distance, Vector2& target) const;
    void setViewOffset(HIGH_PRECISION fullWidth, HIGH_PRECISION fullHeight, HIGH_PRECISION x,
                       HIGH_PRECISION y, HIGH_PRECISION width, HIGH_PRECISION height);
    void clearViewOffset();
    void updateProjectionMatrix();
    using Camera::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;

  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
