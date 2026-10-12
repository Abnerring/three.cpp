#include "lights/PointLight.h"
#include <numbers>
#include <stdexcept>
PointLight::PointLight(const Color& color, double intensity, double distance, double decay) : Light(color, intensity), distance(distance), decay(decay) { type = "PointLight";  }
void PointLight::registerNode(const JSValue& nodeClass) { registerNodeFor("PointLight", nodeClass); }
JSValue PointLight::getLightNode() const { auto node = registeredNodeFor("PointLight"); return node ? *node : Light::getLightNode(); }
PointLight& PointLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const PointLight*>(&source);
    if (!light) throw std::invalid_argument("PointLight::copy requires a PointLight");
    Light::copy(source, recursive);
    distance = light->distance; decay = light->decay;
    shadow = std::dynamic_pointer_cast<PointLightShadow>(light->shadow->clone());
    return *this;
}
std::shared_ptr<Object3D> PointLight::clone(bool recursive) const { auto result = std::make_shared<PointLight>(); result->copy(*this, recursive); return result; }
JsonValue PointLight::toJSON(Object3DJSONContext* meta) const {
    auto data = Light::toJSON(meta); auto& object = data["object"];
    object["distance"] = distance; object["decay"] = decay; object["shadow"] = shadow->toJSON();
    return data;
}
std::optional<JsonValue> PointLight::propertyValue(const std::string& property) const {
    if (property == "isPointLight") return isPointLight;
    if (property == "distance") return distance;
    if (property == "decay") return decay;
    if (property == "power") return getPower();
    return Light::propertyValue(property);
}
void PointLight::dispose() { Object3D::dispose(); const auto retained = shadow; if (!retained) throw JSTypeError("Cannot read properties of null (reading 'dispose')"); retained->dispose(); }
double PointLight::getPower() const { return intensity * 4 * std::numbers::pi; }
void PointLight::setPower(double value) { intensity = value / (4 * std::numbers::pi); }
