#include "lights/HemisphereLight.h"
#include <numbers>
#include <stdexcept>
HemisphereLight::HemisphereLight(const Color& skyColor, const Color& groundColor, double intensity) : Light(skyColor, intensity), groundColor(groundColor) { type = "HemisphereLight"; position.copy(Object3D::DEFAULT_UP); updateMatrix(); }
void HemisphereLight::registerNode(const JSValue& nodeClass) { registerNodeFor("HemisphereLight", nodeClass); }
JSValue HemisphereLight::getLightNode() const { auto node = registeredNodeFor("HemisphereLight"); return node ? *node : Light::getLightNode(); }
HemisphereLight& HemisphereLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const HemisphereLight*>(&source);
    if (!light) throw std::invalid_argument("HemisphereLight::copy requires a HemisphereLight");
    Light::copy(source, recursive);
    groundColor.copy(light->groundColor);
    return *this;
}
std::shared_ptr<Object3D> HemisphereLight::clone(bool recursive) const { auto result = std::make_shared<HemisphereLight>(); result->copy(*this, recursive); return result; }
JsonValue HemisphereLight::toJSON(Object3DJSONContext* meta) const {
    auto data = Light::toJSON(meta); auto& object = data["object"];
    object["groundColor"] = groundColor.getHex();
    return data;
}
std::optional<JsonValue> HemisphereLight::propertyValue(const std::string& property) const {
    if (property == "isHemisphereLight") return isHemisphereLight;

    return Light::propertyValue(property);
}
