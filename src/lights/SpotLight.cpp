#include "lights/SpotLight.h"
#include <numbers>
#include <stdexcept>
SpotLight::SpotLight(const Color& color, double intensity, double distance, double angle, double penumbra, double decay) : Light(color, intensity), distance(distance), angle(angle), penumbra(penumbra), decay(decay) { type = "SpotLight"; position.copy(Object3D::DEFAULT_UP); updateMatrix(); }
void SpotLight::registerNode(const JSValue& nodeClass) { registerNodeFor("SpotLight", nodeClass); }
JSValue SpotLight::getLightNode() const { auto node = registeredNodeFor("SpotLight"); return node ? *node : Light::getLightNode(); }
SpotLight& SpotLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const SpotLight*>(&source);
    if (!light) throw std::invalid_argument("SpotLight::copy requires a SpotLight");
    Light::copy(source, recursive);
    distance = light->distance; angle = light->angle; penumbra = light->penumbra; decay = light->decay;
    target = light->target->clone(); map = light->map;
    shadow = std::dynamic_pointer_cast<SpotLightShadow>(light->shadow->clone());
    return *this;
}
std::shared_ptr<Object3D> SpotLight::clone(bool recursive) const { auto result = std::make_shared<SpotLight>(); result->copy(*this, recursive); return result; }
JsonValue SpotLight::toJSON(Object3DJSONContext* meta) const {
    auto data = Light::toJSON(meta); auto& object = data["object"];
    object["distance"] = distance; object["angle"] = angle; object["decay"] = decay; object["penumbra"] = penumbra;
    object["target"] = target->uuid;
    if (const auto retainedMap = map; retainedMap && retainedMap->isTexture) object["map"] = retainedMap->toJSON(meta).at("uuid");
    object["shadow"] = shadow->toJSON();
    return data;
}
std::optional<JsonValue> SpotLight::propertyValue(const std::string& property) const {
    if (property == "isSpotLight") return isSpotLight;
    if (property == "distance") return distance;
    if (property == "angle") return angle;
    if (property == "penumbra") return penumbra;
    if (property == "decay") return decay;
    if (property == "power") return getPower();
    return Light::propertyValue(property);
}
void SpotLight::dispose() { Object3D::dispose(); const auto retained = shadow; if (!retained) throw JSTypeError("Cannot read properties of null (reading 'dispose')"); retained->dispose(); }
double SpotLight::getPower() const { return intensity * std::numbers::pi; }
void SpotLight::setPower(double value) { intensity = value / (std::numbers::pi); }
