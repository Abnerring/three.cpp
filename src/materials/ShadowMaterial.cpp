#include "materials/ShadowMaterial.h"

ShadowMaterial::ShadowMaterial(const JsonValue& parameters) {
    type = "ShadowMaterial";
    transparent = true;
    setValues(parameters);
}
void ShadowMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isShadowMaterial", &isShadowMaterial);
    visitor("color", &color);
    visitor("fog", &fog);
}
Material& ShadowMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const ShadowMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> ShadowMaterial::clone() const {
    auto result = std::make_shared<ShadowMaterial>();
    result->copy(*this);
    return result;
}
