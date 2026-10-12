#include "materials/SpriteMaterial.h"

SpriteMaterial::SpriteMaterial(const JsonValue& parameters) {
    type = "SpriteMaterial";
    transparent = true;
    setValues(parameters);
}
void SpriteMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isSpriteMaterial", &isSpriteMaterial);
    visitor("color", &color);
    visitor("map", &map);
    visitor("alphaMap", &alphaMap);
    visitor("rotation", &rotation);
    visitor("sizeAttenuation", &sizeAttenuation);
    visitor("fog", &fog);
}
Material& SpriteMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const SpriteMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    alphaMap = s.alphaMap;
    rotation = s.rotation;
    sizeAttenuation = s.sizeAttenuation;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> SpriteMaterial::clone() const {
    auto result = std::make_shared<SpriteMaterial>();
    result->copy(*this);
    return result;
}
