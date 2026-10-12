#include "lights/RectAreaLight.h"
#include <numbers>
#include <stdexcept>
RectAreaLight::RectAreaLight(const Color& color, double intensity, double width, double height) : Light(color, intensity), width(width), height(height) { type = "RectAreaLight";  }
void RectAreaLight::registerNode(const JSValue& nodeClass) { registerNodeFor("RectAreaLight", nodeClass); }
JSValue RectAreaLight::getLightNode() const { auto node = registeredNodeFor("RectAreaLight"); return node ? *node : Light::getLightNode(); }
RectAreaLight& RectAreaLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const RectAreaLight*>(&source);
    if (!light) throw std::invalid_argument("RectAreaLight::copy requires a RectAreaLight");
    (void)recursive; Light::copy(source);
    width = light->width; height = light->height;
    return *this;
}
std::shared_ptr<Object3D> RectAreaLight::clone(bool recursive) const { auto result = std::make_shared<RectAreaLight>(); result->copy(*this, recursive); return result; }
JsonValue RectAreaLight::toJSON(Object3DJSONContext* meta) const {
    auto data = Light::toJSON(meta); auto& object = data["object"];
    object["width"] = width; object["height"] = height;
    return data;
}
std::optional<JsonValue> RectAreaLight::propertyValue(const std::string& property) const {
    if (property == "isRectAreaLight") return isRectAreaLight;
    if (property == "width") return width;
    if (property == "height") return height;
    if (property == "power") return getPower();
    return Light::propertyValue(property);
}
double RectAreaLight::getPower() const { return intensity * width * height * std::numbers::pi; }
void RectAreaLight::setPower(double value) { intensity = value / (width * height * std::numbers::pi); }
