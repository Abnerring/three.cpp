#include "cameras/OrthographicCamera.h"
#include <stdexcept>

OrthographicCamera::OrthographicCamera(HIGH_PRECISION left, HIGH_PRECISION right,
                                       HIGH_PRECISION top, HIGH_PRECISION bottom,
                                       HIGH_PRECISION near, HIGH_PRECISION far)
    : left(left), right(right), top(top), bottom(bottom), near(near), far(far) {
    type = "OrthographicCamera";
    updateProjectionMatrix();
}
std::optional<JsonValue> OrthographicCamera::propertyValue(const std::string& property) const {
    if (property == "isOrthographicCamera")
        return isOrthographicCamera;
    if (property == "zoom")
        return zoom;
    if (property == "left")
        return left;
    if (property == "right")
        return right;
    if (property == "top")
        return top;
    if (property == "bottom")
        return bottom;
    if (property == "near")
        return near;
    if (property == "far")
        return far;
    return Camera::propertyValue(property);
}
OrthographicCamera& OrthographicCamera::copy(const Object3D& source, bool recursive) {
    const auto* camera = dynamic_cast<const OrthographicCamera*>(&source);
    if (!camera)
        throw std::invalid_argument("OrthographicCamera::copy requires an OrthographicCamera");
    Camera::copy(source, recursive);
    left = camera->left;
    right = camera->right;
    top = camera->top;
    bottom = camera->bottom;
    near = camera->near;
    far = camera->far;
    zoom = camera->zoom;
    view = camera->view;
    return *this;
}
std::shared_ptr<Object3D> OrthographicCamera::clone(bool) const {
    auto result = std::make_shared<OrthographicCamera>();
    result->copy(*this);
    return result;
}
void OrthographicCamera::setViewOffset(HIGH_PRECISION fullWidth, HIGH_PRECISION fullHeight,
                                       HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION width,
                                       HIGH_PRECISION height) {
    view = CameraView{true, fullWidth, fullHeight, x, y, width, height};
    updateProjectionMatrix();
}
void OrthographicCamera::clearViewOffset() {
    if (view)
        view->enabled = false;
    updateProjectionMatrix();
}
void OrthographicCamera::updateProjectionMatrix() {
    const auto dx = (right - left) / (2 * zoom), dy = (top - bottom) / (2 * zoom);
    const auto cx = (right + left) / 2, cy = (top + bottom) / 2;
    auto l = cx - dx, r = cx + dx, t = cy + dy, b = cy - dy;
    if (view && view->enabled) {
        const auto scaleW = (right - left) / view->fullWidth / zoom;
        const auto scaleH = (top - bottom) / view->fullHeight / zoom;
        l += scaleW * view->offsetX;
        r = l + scaleW * view->width;
        t -= scaleH * view->offsetY;
        b = t - scaleH * view->height;
    }
    projectionMatrix.makeOrthographic(l, r, t, b, near, far, coordinateSystem, reversedDepth);
    projectionMatrixInverse.copy(projectionMatrix);
    projectionMatrixInverse.invert();
}
JsonValue OrthographicCamera::toJSON(Object3DJSONContext* meta) const {
    auto output = Camera::toJSON(meta);
    auto& object = output["object"];
    object["zoom"] = zoom;
    object["left"] = left;
    object["right"] = right;
    object["top"] = top;
    object["bottom"] = bottom;
    object["near"] = near;
    object["far"] = far;
    if (view)
        object["view"] = view->toJSON();
    return output;
}
