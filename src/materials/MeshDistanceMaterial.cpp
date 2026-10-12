#include "materials/MeshDistanceMaterial.h"

MeshDistanceMaterial::MeshDistanceMaterial(const JsonValue& parameters) {
    type = "MeshDistanceMaterial";
    setValues(parameters);
}
void MeshDistanceMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshDistanceMaterial", &isMeshDistanceMaterial);
    visitor("map", &map);
    visitor("alphaMap", &alphaMap);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
}
Material& MeshDistanceMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshDistanceMaterial&>(source);
    Material::copy(source);
    map = s.map;
    alphaMap = s.alphaMap;
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    return *this;
}
std::shared_ptr<Material> MeshDistanceMaterial::clone() const {
    auto result = std::make_shared<MeshDistanceMaterial>();
    result->copy(*this);
    return result;
}
