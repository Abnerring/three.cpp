#include "cameras/PerspectiveCamera.h"
#include "math/JSNumberMath.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

PerspectiveCamera::PerspectiveCamera(HIGH_PRECISION fov, HIGH_PRECISION aspect, HIGH_PRECISION near,
                                     HIGH_PRECISION far)
    : fov(fov), near(near), far(far), aspect(aspect) {
    type = "PerspectiveCamera";
    updateProjectionMatrix();
}
std::optional<JsonValue> PerspectiveCamera::propertyValue(const std::string& property) const {
    if (property == "isPerspectiveCamera")
        return isPerspectiveCamera;
    if (property == "fov")
        return fov;
    if (property == "zoom")
        return zoom;
    if (property == "near")
        return near;
    if (property == "far")
        return far;
    if (property == "focus")
        return focus;
    if (property == "aspect")
        return aspect;
    if (property == "filmGauge")
        return filmGauge;
    if (property == "filmOffset")
        return filmOffset;
    return Camera::propertyValue(property);
}
PerspectiveCamera& PerspectiveCamera::copy(const Object3D& source, bool recursive) {
    const auto* camera = dynamic_cast<const PerspectiveCamera*>(&source);
    if (!camera)
        throw std::invalid_argument("PerspectiveCamera::copy requires a PerspectiveCamera");
    Camera::copy(source, recursive);
    fov = camera->fov;
    zoom = camera->zoom;
    near = camera->near;
    far = camera->far;
    focus = camera->focus;
    aspect = camera->aspect;
    view = camera->view;
    filmGauge = camera->filmGauge;
    filmOffset = camera->filmOffset;
    return *this;
}
std::shared_ptr<Object3D> PerspectiveCamera::clone(bool) const {
    auto result = std::make_shared<PerspectiveCamera>();
    result->copy(*this);
    return result;
}
void PerspectiveCamera::setFocalLength(HIGH_PRECISION focalLength) {
    const auto vExtentSlope = HIGH_PRECISION(0.5) * getFilmHeight() / focalLength;
    fov = 180 / MATH_PI * 2 * JSNumberMath::atan(vExtentSlope);
    updateProjectionMatrix();
}
HIGH_PRECISION PerspectiveCamera::getFocalLength() const {
    return HIGH_PRECISION(0.5) * getFilmHeight() /
           JSNumberMath::tan(MATH_PI / 180 * HIGH_PRECISION(0.5) * fov);
}
HIGH_PRECISION PerspectiveCamera::getEffectiveFOV() const {
    return 180 / MATH_PI * 2 *
           JSNumberMath::atan(JSNumberMath::tan(MATH_PI / 180 * HIGH_PRECISION(0.5) * fov) / zoom);
}
HIGH_PRECISION PerspectiveCamera::getFilmWidth() const {
    return filmGauge * std::min(aspect, HIGH_PRECISION(1));
}
HIGH_PRECISION PerspectiveCamera::getFilmHeight() const {
    return filmGauge / std::max(aspect, HIGH_PRECISION(1));
}
void PerspectiveCamera::getViewBounds(HIGH_PRECISION distance, Vector2& minTarget,
                                      Vector2& maxTarget) const {
    Vector3 v{-1, -1, HIGH_PRECISION(0.5)};
    v.applyMatrix4(projectionMatrixInverse);
    minTarget.set(v.x, v.y);
    minTarget.multiplyScalar(-distance / v.z);
    v.set(1, 1, HIGH_PRECISION(0.5));
    v.applyMatrix4(projectionMatrixInverse);
    maxTarget.set(v.x, v.y);
    maxTarget.multiplyScalar(-distance / v.z);
}
Vector2& PerspectiveCamera::getViewSize(HIGH_PRECISION distance, Vector2& target) const {
    Vector2 min, max;
    getViewBounds(distance, min, max);
    target.subVectors(max, min);
    return target;
}
void PerspectiveCamera::setViewOffset(HIGH_PRECISION fullWidth, HIGH_PRECISION fullHeight,
                                      HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION width,
                                      HIGH_PRECISION height) {
    aspect = fullWidth / fullHeight;
    view = CameraView{true, fullWidth, fullHeight, x, y, width, height};
    updateProjectionMatrix();
}
void PerspectiveCamera::clearViewOffset() {
    if (view)
        view->enabled = false;
    updateProjectionMatrix();
}
void PerspectiveCamera::updateProjectionMatrix() {
    auto top = near * JSNumberMath::tan(MATH_PI / 180 * HIGH_PRECISION(0.5) * fov) / zoom;
    auto height = 2 * top;
    auto width = aspect * height;
    auto left = HIGH_PRECISION(-0.5) * width;
    if (view && view->enabled) {
        left += view->offsetX * width / view->fullWidth;
        top -= view->offsetY * height / view->fullHeight;
        width *= view->width / view->fullWidth;
        height *= view->height / view->fullHeight;
    }
    if (filmOffset != 0)
        left += near * filmOffset / getFilmWidth();
    projectionMatrix.makePerspective(left, left + width, top, top - height, near, far,
                                     coordinateSystem, reversedDepth);
    projectionMatrixInverse.copy(projectionMatrix);
    projectionMatrixInverse.invert();
}
JsonValue PerspectiveCamera::toJSON(Object3DJSONContext* meta) const {
    auto output = Camera::toJSON(meta);
    auto& object = output["object"];
    object["fov"] = fov;
    object["zoom"] = zoom;
    object["near"] = near;
    object["far"] = far;
    object["focus"] = focus;
    object["aspect"] = aspect;
    if (view)
        object["view"] = view->toJSON();
    object["filmGauge"] = filmGauge;
    object["filmOffset"] = filmOffset;
    return output;
}
