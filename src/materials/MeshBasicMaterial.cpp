#include "materials/MeshBasicMaterial.h"

MeshBasicMaterial::MeshBasicMaterial(const JsonValue& parameters) {
    isMeshBasicMaterial = true;
    type = "MeshBasicMaterial";
    setValues(parameters);
}
MeshBasicMaterial::MeshBasicMaterial(const Color& value) : MeshBasicMaterial() { color.copy(value); }
void MeshBasicMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshBasicMaterial", &isMeshBasicMaterial);
    visitor("color", &color);
    visitor("map", &map);
    visitor("lightMap", &lightMap);
    visitor("lightMapIntensity", &lightMapIntensity);
    visitor("aoMap", &aoMap);
    visitor("aoMapIntensity", &aoMapIntensity);
    visitor("specularMap", &specularMap);
    visitor("alphaMap", &alphaMap);
    visitor("envMap", &envMap);
    visitor("envMapRotation", &envMapRotation);
    visitor("combine", &combine);
    visitor("reflectivity", &reflectivity);
    visitor("refractionRatio", &refractionRatio);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("wireframeLinecap", &wireframeLinecap);
    visitor("wireframeLinejoin", &wireframeLinejoin);
    visitor("fog", &fog);
}
Material& MeshBasicMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshBasicMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    lightMap = s.lightMap;
    lightMapIntensity = s.lightMapIntensity;
    aoMap = s.aoMap;
    aoMapIntensity = s.aoMapIntensity;
    specularMap = s.specularMap;
    alphaMap = s.alphaMap;
    envMap = s.envMap;
    envMapRotation.copy( s.envMapRotation );
    combine = s.combine;
    reflectivity = s.reflectivity;
    refractionRatio = s.refractionRatio;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    wireframeLinecap = s.wireframeLinecap;
    wireframeLinejoin = s.wireframeLinejoin;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshBasicMaterial::clone() const {
    auto result = std::make_shared<MeshBasicMaterial>();
    result->copy(*this);
    return result;
}
