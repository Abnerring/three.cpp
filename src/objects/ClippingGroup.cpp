#include "objects/ClippingGroup.h"

ClippingGroup::ClippingGroup() { isClippingGroup = true; }
std::shared_ptr<Object3D> ClippingGroup::clone(bool recursive) const {
    auto result = std::make_shared<ClippingGroup>();
    result->copy(*this, recursive);
    return result;
}
std::optional<JsonValue> ClippingGroup::propertyValue(const std::string& property) const {
    if (property == "isClippingGroup") return isClippingGroup;
    if (property == "enabled") return enabled.toJSON();
    if (property == "clipIntersection") return clipIntersection.toJSON();
    if (property == "clipShadows") return clipShadows.toJSON();
    return Group::propertyValue(property);
}
