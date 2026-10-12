#include "materials/MeshToonMaterial.h"

MeshToonMaterial::MeshToonMaterial(const JsonValue& parameters) {
    type = "MeshToonMaterial";
    setValues(parameters);
}
void MeshToonMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshToonMaterial", &isMeshToonMaterial);
    visitor("defines", &defines);
    visitor("color", &color);
    visitor("map", &map);
    visitor("gradientMap", &gradientMap);
    visitor("lightMap", &lightMap);
    visitor("lightMapIntensity", &lightMapIntensity);
    visitor("aoMap", &aoMap);
    visitor("aoMapIntensity", &aoMapIntensity);
    visitor("emissive", &emissive);
    visitor("emissiveIntensity", &emissiveIntensity);
    visitor("emissiveMap", &emissiveMap);
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
    visitor("wireframeLinecap", &wireframeLinecap);
    visitor("wireframeLinejoin", &wireframeLinejoin);
    visitor("fog", &fog);
}
Material& MeshToonMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshToonMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    gradientMap = s.gradientMap;
    lightMap = s.lightMap;
    lightMapIntensity = s.lightMapIntensity;
    aoMap = s.aoMap;
    aoMapIntensity = s.aoMapIntensity;
    emissive.copy( s.emissive );
    emissiveMap = s.emissiveMap;
    emissiveIntensity = s.emissiveIntensity;
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
    wireframeLinecap = s.wireframeLinecap;
    wireframeLinejoin = s.wireframeLinejoin;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshToonMaterial::clone() const {
    auto result = std::make_shared<MeshToonMaterial>();
    result->copy(*this);
    return result;
}
