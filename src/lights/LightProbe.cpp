#include "lights/LightProbe.h"
#include <numbers>
#include <stdexcept>
LightProbe::LightProbe(std::shared_ptr<SphericalHarmonics3> sh, double intensity) : Light(Color(), intensity), sh(std::move(sh)) { type = "LightProbe";  }
void LightProbe::registerNode(const JSValue& nodeClass) { registerNodeFor("LightProbe", nodeClass); }
JSValue LightProbe::getLightNode() const { auto node = registeredNodeFor("LightProbe"); return node ? *node : Light::getLightNode(); }
LightProbe& LightProbe::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const LightProbe*>(&source);
    if (!light) throw std::invalid_argument("LightProbe::copy requires a LightProbe");
    (void)recursive; Light::copy(source);
    if (!sh || !light->sh) throw JSTypeError("LightProbe.sh is not a SphericalHarmonics3");
    sh->copy(*light->sh);
    return *this;
}
std::shared_ptr<Object3D> LightProbe::clone(bool recursive) const { auto result = std::make_shared<LightProbe>(); result->copy(*this, recursive); return result; }
JsonValue LightProbe::toJSON(Object3DJSONContext* meta) const {
    auto data = Light::toJSON(meta); auto& object = data["object"];
    if (!sh) throw JSTypeError("LightProbe.sh is not a SphericalHarmonics3");
    JsonValue::Array coefficients; for (double value : sh->toArray()) coefficients.emplace_back(value);
    object["sh"] = std::move(coefficients);
    return data;
}
std::optional<JsonValue> LightProbe::propertyValue(const std::string& property) const {
    if (property == "isLightProbe") return isLightProbe;

    return Light::propertyValue(property);
}
