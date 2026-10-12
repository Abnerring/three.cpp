#include "lights/Light.h"
#include <unordered_map>
#include <stdexcept>

namespace { std::unordered_map<std::string, JSValue>& nodeClasses() { static std::unordered_map<std::string, JSValue> values; return values; } }
Light::Light(const Color& color, double intensity) : color(color), intensity(intensity) { isLight = true; type = "Light"; }
void Light::registerNodeFor(const std::string& className, const JSValue& nodeClass) { nodeClasses().insert_or_assign(className, nodeClass); }
std::optional<JSValue> Light::registeredNodeFor(const std::string& className) { const auto& values = nodeClasses(); const auto found = values.find(className); return found == values.end() ? std::nullopt : std::optional<JSValue>(found->second); }
void Light::registerNode(const JSValue& nodeClass) { registerNodeFor("Light", nodeClass); }
JSValue Light::getLightNode() const { return registeredNodeFor("Light").value_or(JSValue()); }
Light& Light::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const Light*>(&source);
    if (!light) throw std::invalid_argument("Light::copy requires a Light");
    Object3D::copy(source, recursive);
    color.copy(light->color); intensity = light->intensity;
    return *this;
}
std::shared_ptr<Object3D> Light::clone(bool recursive) const { auto result = std::make_shared<Light>(); result->copy(*this, recursive); return result; }
JsonValue Light::toJSON(Object3DJSONContext* meta) const { auto data = Object3D::toJSON(meta); data["object"]["color"] = color.getHex(); data["object"]["intensity"] = intensity; return data; }
std::optional<JsonValue> Light::propertyValue(const std::string& property) const { if (property == "intensity") return intensity; return Object3D::propertyValue(property); }
