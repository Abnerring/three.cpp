#include "materials/MeshDepthMaterial.h"

MeshDepthMaterial::MeshDepthMaterial(const JsonValue& parameters) {
    type = "MeshDepthMaterial";
    setValues(parameters);
}
void MeshDepthMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshDepthMaterial", &isMeshDepthMaterial);
    visitor("depthPacking", &depthPacking);
    visitor("map", &map);
    visitor("alphaMap", &alphaMap);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
}
Material& MeshDepthMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshDepthMaterial&>(source);
    Material::copy(source);
    depthPacking = s.depthPacking;
    map = s.map;
    alphaMap = s.alphaMap;
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    return *this;
}
std::shared_ptr<Material> MeshDepthMaterial::clone() const {
    auto result = std::make_shared<MeshDepthMaterial>();
    result->copy(*this);
    return result;
}
