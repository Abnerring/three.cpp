#include "materials/MeshPhongMaterial.h"

MeshPhongMaterial::MeshPhongMaterial(const JsonValue& parameters) {
    type = "MeshPhongMaterial";
    setValues(parameters);
}
void MeshPhongMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshPhongMaterial", &isMeshPhongMaterial);
    visitor("color", &color);
    visitor("specular", &specular);
    visitor("shininess", &shininess);
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
    visitor("specularMap", &specularMap);
    visitor("alphaMap", &alphaMap);
    visitor("envMap", &envMap);
    visitor("envMapRotation", &envMapRotation);
    visitor("combine", &combine);
    visitor("reflectivity", &reflectivity);
    visitor("envMapIntensity", &envMapIntensity);
    visitor("refractionRatio", &refractionRatio);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("wireframeLinecap", &wireframeLinecap);
    visitor("wireframeLinejoin", &wireframeLinejoin);
    visitor("flatShading", &flatShading);
    visitor("fog", &fog);
}
Material& MeshPhongMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshPhongMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    specular.copy( s.specular );
    shininess = s.shininess;
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
    specularMap = s.specularMap;
    alphaMap = s.alphaMap;
    envMap = s.envMap;
    envMapRotation.copy( s.envMapRotation );
    combine = s.combine;
    reflectivity = s.reflectivity;
    envMapIntensity = s.envMapIntensity;
    refractionRatio = s.refractionRatio;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    wireframeLinecap = s.wireframeLinecap;
    wireframeLinejoin = s.wireframeLinejoin;
    flatShading = s.flatShading;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshPhongMaterial::clone() const {
    auto result = std::make_shared<MeshPhongMaterial>();
    result->copy(*this);
    return result;
}
