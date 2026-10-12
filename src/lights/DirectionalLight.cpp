#include "lights/DirectionalLight.h"
#include <numbers>
#include <stdexcept>
DirectionalLight::DirectionalLight(const Color& color, double intensity) : Light(color, intensity) { type = "DirectionalLight"; position.copy(Object3D::DEFAULT_UP); updateMatrix(); }
void DirectionalLight::registerNode(const JSValue& nodeClass) { registerNodeFor("DirectionalLight", nodeClass); }
JSValue DirectionalLight::getLightNode() const { auto node = registeredNodeFor("DirectionalLight"); return node ? *node : Light::getLightNode(); }
DirectionalLight& DirectionalLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const DirectionalLight*>(&source);
    if (!light) throw std::invalid_argument("DirectionalLight::copy requires a DirectionalLight");
    (void)recursive; Light::copy(source);
    target = light->target->clone();
    shadow = std::dynamic_pointer_cast<DirectionalLightShadow>(light->shadow->clone());
    return *this;
}
std::shared_ptr<Object3D> DirectionalLight::clone(bool recursive) const { auto result = std::make_shared<DirectionalLight>(); result->copy(*this, recursive); return result; }
JsonValue DirectionalLight::toJSON(Object3DJSONContext* meta) const {
    auto data = Light::toJSON(meta); auto& object = data["object"];
    object["shadow"] = shadow->toJSON(); object["target"] = target->uuid;
    return data;
}
std::optional<JsonValue> DirectionalLight::propertyValue(const std::string& property) const {
    if (property == "isDirectionalLight") return isDirectionalLight;

    return Light::propertyValue(property);
}
void DirectionalLight::dispose() { Object3D::dispose(); const auto retained = shadow; if (!retained) throw JSTypeError("Cannot read properties of null (reading 'dispose')"); retained->dispose(); }
