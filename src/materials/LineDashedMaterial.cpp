#include "materials/LineDashedMaterial.h"

LineDashedMaterial::LineDashedMaterial(const JsonValue& parameters) {
    type = "LineDashedMaterial";
    setValues(parameters);
}
void LineDashedMaterial::visitProperties(const PropertyVisitor& visitor) {
    LineBasicMaterial::visitProperties(visitor);
    visitor("isLineDashedMaterial", &isLineDashedMaterial);
    visitor("scale", &scale);
    visitor("dashSize", &dashSize);
    visitor("gapSize", &gapSize);
}
Material& LineDashedMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const LineDashedMaterial&>(source);
    LineBasicMaterial::copy(source);
    scale = s.scale;
    dashSize = s.dashSize;
    gapSize = s.gapSize;
    return *this;
}
std::shared_ptr<Material> LineDashedMaterial::clone() const {
    auto result = std::make_shared<LineDashedMaterial>();
    result->copy(*this);
    return result;
}
