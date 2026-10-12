#include "materials/LineBasicMaterial.h"

LineBasicMaterial::LineBasicMaterial(const JsonValue& parameters) {
    type = "LineBasicMaterial";
    setValues(parameters);
}
void LineBasicMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isLineBasicMaterial", &isLineBasicMaterial);
    visitor("color", &color);
    visitor("map", &map);
    visitor("linewidth", &linewidth);
    visitor("linecap", &linecap);
    visitor("linejoin", &linejoin);
    visitor("fog", &fog);
}
Material& LineBasicMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const LineBasicMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    linewidth = s.linewidth;
    linecap = s.linecap;
    linejoin = s.linejoin;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> LineBasicMaterial::clone() const {
    auto result = std::make_shared<LineBasicMaterial>();
    result->copy(*this);
    return result;
}
