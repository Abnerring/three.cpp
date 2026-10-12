#include "materials/MeshNormalMaterial.h"

MeshNormalMaterial::MeshNormalMaterial(const JsonValue& parameters) {
    type = "MeshNormalMaterial";
    setValues(parameters);
}
void MeshNormalMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshNormalMaterial", &isMeshNormalMaterial);
    visitor("bumpMap", &bumpMap);
    visitor("bumpScale", &bumpScale);
    visitor("normalMap", &normalMap);
    visitor("normalMapType", &normalMapType);
    visitor("normalScale", &normalScale);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("flatShading", &flatShading);
}
Material& MeshNormalMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshNormalMaterial&>(source);
    Material::copy(source);
    bumpMap = s.bumpMap;
    bumpScale = s.bumpScale;
    normalMap = s.normalMap;
    normalMapType = s.normalMapType;
    normalScale.copy( s.normalScale );
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    flatShading = s.flatShading;
    return *this;
}
std::shared_ptr<Material> MeshNormalMaterial::clone() const {
    auto result = std::make_shared<MeshNormalMaterial>();
    result->copy(*this);
    return result;
}
