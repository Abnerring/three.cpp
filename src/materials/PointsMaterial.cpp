#include "materials/PointsMaterial.h"

PointsMaterial::PointsMaterial(const JsonValue& parameters) {
    type = "PointsMaterial";
    setValues(parameters);
}
void PointsMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isPointsMaterial", &isPointsMaterial);
    visitor("color", &color);
    visitor("map", &map);
    visitor("alphaMap", &alphaMap);
    visitor("size", &size);
    visitor("sizeAttenuation", &sizeAttenuation);
    visitor("fog", &fog);
}
Material& PointsMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const PointsMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    alphaMap = s.alphaMap;
    size = s.size;
    sizeAttenuation = s.sizeAttenuation;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> PointsMaterial::clone() const {
    auto result = std::make_shared<PointsMaterial>();
    result->copy(*this);
    return result;
}
