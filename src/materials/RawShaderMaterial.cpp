#include "materials/RawShaderMaterial.h"

RawShaderMaterial::RawShaderMaterial(const JsonValue& parameters) : ShaderMaterial(parameters) {
    type = "RawShaderMaterial";
}
void RawShaderMaterial::visitProperties(const PropertyVisitor& visitor) {
    ShaderMaterial::visitProperties(visitor);
    visitor("isRawShaderMaterial", &isRawShaderMaterial);
}
std::shared_ptr<Material> RawShaderMaterial::clone() const {
    auto result = std::make_shared<RawShaderMaterial>();
    result->copy(*this);
    return result;
}
