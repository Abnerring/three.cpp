#include "materials/MeshMatcapMaterial.h"

MeshMatcapMaterial::MeshMatcapMaterial(const JsonValue& parameters) {
    type = "MeshMatcapMaterial";
    setValues(parameters);
}
void MeshMatcapMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshMatcapMaterial", &isMeshMatcapMaterial);
    visitor("defines", &defines);
    visitor("color", &color);
    visitor("matcap", &matcap);
    visitor("map", &map);
    visitor("bumpMap", &bumpMap);
    visitor("bumpScale", &bumpScale);
    visitor("normalMap", &normalMap);
    visitor("normalMapType", &normalMapType);
    visitor("normalScale", &normalScale);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
    visitor("alphaMap", &alphaMap);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("flatShading", &flatShading);
    visitor("fog", &fog);
}
Material& MeshMatcapMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshMatcapMaterial&>(source);
    Material::copy(source);
    defines = JSValue::fromJSON(JsonValue::Object{{"MATCAP", ""}});
    color.copy( s.color );
    matcap = s.matcap;
    map = s.map;
    bumpMap = s.bumpMap;
    bumpScale = s.bumpScale;
    normalMap = s.normalMap;
    normalMapType = s.normalMapType;
    normalScale.copy( s.normalScale );
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    alphaMap = s.alphaMap;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    flatShading = s.flatShading;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshMatcapMaterial::clone() const {
    auto result = std::make_shared<MeshMatcapMaterial>();
    result->copy(*this);
    return result;
}
