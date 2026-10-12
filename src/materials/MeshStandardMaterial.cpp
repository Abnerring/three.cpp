#include "materials/MeshStandardMaterial.h"

MeshStandardMaterial::MeshStandardMaterial(const JsonValue& parameters) {
    type = "MeshStandardMaterial";
    setValues(parameters);
}
void MeshStandardMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshStandardMaterial", &isMeshStandardMaterial);
    visitor("defines", &defines);
    visitor("color", &color);
    visitor("roughness", &roughness);
    visitor("metalness", &metalness);
    visitor("map", &map);
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
    visitor("roughnessMap", &roughnessMap);
    visitor("metalnessMap", &metalnessMap);
    visitor("alphaMap", &alphaMap);
    visitor("envMap", &envMap);
    visitor("envMapRotation", &envMapRotation);
    visitor("envMapIntensity", &envMapIntensity);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("wireframeLinecap", &wireframeLinecap);
    visitor("wireframeLinejoin", &wireframeLinejoin);
    visitor("flatShading", &flatShading);
    visitor("fog", &fog);
}
Material& MeshStandardMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshStandardMaterial&>(source);
    Material::copy(source);
    defines = JSValue::fromJSON(JsonValue::Object{{"STANDARD", ""}});
    color.copy( s.color );
    roughness = s.roughness;
    metalness = s.metalness;
    map = s.map;
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
    roughnessMap = s.roughnessMap;
    metalnessMap = s.metalnessMap;
    alphaMap = s.alphaMap;
    envMap = s.envMap;
    envMapRotation.copy( s.envMapRotation );
    envMapIntensity = s.envMapIntensity;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    wireframeLinecap = s.wireframeLinecap;
    wireframeLinejoin = s.wireframeLinejoin;
    flatShading = s.flatShading;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshStandardMaterial::clone() const {
    auto result = std::make_shared<MeshStandardMaterial>();
    result->copy(*this);
    return result;
}
