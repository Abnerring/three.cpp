#include "cameras/Camera.h"
#include <stdexcept>

JsonValue CameraView::toJSON() const {
    return JsonValue::Object{{"enabled", enabled},       {"fullWidth", fullWidth},
                             {"fullHeight", fullHeight}, {"offsetX", offsetX},
                             {"offsetY", offsetY},       {"width", width},
                             {"height", height}};
}

Camera::Camera() {
    isCamera = true;
    type = "Camera";
}
std::optional<JsonValue> Camera::propertyValue(const std::string& property) const {
    if (property == "isCamera")
        return isCamera;
    if (property == "coordinateSystem")
        return coordinateSystem;
    if (property == "reversedDepth")
        return reversedDepth;
    return Object3D::propertyValue(property);
}
Camera& Camera::copy(const Object3D& source, bool recursive) {
    const auto* camera = dynamic_cast<const Camera*>(&source);
    if (!camera)
        throw std::invalid_argument("Camera::copy requires a Camera");
    Object3D::copy(source, recursive);
    matrixWorldInverse.copy(camera->matrixWorldInverse);
    projectionMatrix.copy(camera->projectionMatrix);
    projectionMatrixInverse.copy(camera->projectionMatrixInverse);
    coordinateSystem = camera->coordinateSystem;
    return *this;
}
Vector3& Camera::getWorldDirection(Vector3& target) {
    Object3D::getWorldDirection(target);
    target.negate();
    return target;
}
void Camera::updateViewMatrix() {
    Vector3 p, s;
    Quaternion q;
    matrixWorld.decompose(p, q, s);
    // three.js r186 excludes scale from camera view matrices to conform to glTF.
    if (s.x() == 1 && s.y() == 1 && s.z() == 1)
        matrixWorldInverse.copy(matrixWorld);
    else {
        s.set(1, 1, 1);
        matrixWorldInverse.compose(p, q, s);
    }
    matrixWorldInverse.invert();
}
void Camera::updateMatrixWorld(bool force) {
    Object3D::updateMatrixWorld(force);
    updateViewMatrix();
}
void Camera::updateWorldMatrix(bool updateParents, bool updateChildren, bool force) {
    Object3D::updateWorldMatrix(updateParents, updateChildren, force);
    updateViewMatrix();
}
std::shared_ptr<Object3D> Camera::clone(bool recursive) const {
    auto result = std::make_shared<Camera>();
    result->copy(*this, recursive);
    return result;
}
