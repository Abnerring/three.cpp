#include "lights/AmbientLight.h"
#include <numbers>
#include <stdexcept>
AmbientLight::AmbientLight(const Color& color, double intensity) : Light(color, intensity) { type = "AmbientLight";  }
void AmbientLight::registerNode(const JSValue& nodeClass) { registerNodeFor("AmbientLight", nodeClass); }
JSValue AmbientLight::getLightNode() const { auto node = registeredNodeFor("AmbientLight"); return node ? *node : Light::getLightNode(); }
AmbientLight& AmbientLight::copy(const Object3D& source, bool recursive) {
    Light::copy(source, recursive);

    return *this;
}
std::shared_ptr<Object3D> AmbientLight::clone(bool recursive) const { auto result = std::make_shared<AmbientLight>(); result->copy(*this, recursive); return result; }
JsonValue AmbientLight::toJSON(Object3DJSONContext* meta) const {
    return Light::toJSON(meta);
}
std::optional<JsonValue> AmbientLight::propertyValue(const std::string& property) const {
    if (property == "isAmbientLight") return isAmbientLight;

    return Light::propertyValue(property);
}
