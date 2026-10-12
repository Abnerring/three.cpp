#include "cameras/ArrayCamera.h"

ArrayCamera::ArrayCamera(SharedArray<std::shared_ptr<Camera>> array) : cameras(std::move(array)) {
    isArrayCamera = true;
    isMultiViewCamera = false;
}
std::shared_ptr<Object3D> ArrayCamera::clone(bool) const {
    auto result = std::make_shared<ArrayCamera>();
    // Camera.clone() accepts no recursive parameter in the upstream module.
    result->copy(*this);
    return result;
}
std::optional<JsonValue> ArrayCamera::propertyValue(const std::string& property) const {
    if (property == "isArrayCamera") return isArrayCamera;
    if (property == "isMultiViewCamera") return isMultiViewCamera;
    return PerspectiveCamera::propertyValue(property);
}
