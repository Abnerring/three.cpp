#include "materials/MeshBasicMaterial.h"
MeshBasicMaterial::MeshBasicMaterial(const JsonValue& p) {
    isMeshBasicMaterial = true;
    type = "MeshBasicMaterial";
    setValues(p);
}
MeshBasicMaterial::MeshBasicMaterial(const Color& c) : MeshBasicMaterial() {
    color.copy(c);
}
void MeshBasicMaterial::setValues(const JsonValue& p) {
    if (p.isNull())
        return;
    JsonValue base = JsonValue::Object{};
    for (const auto& [key, value] : p.get<JsonValue::Object>()) {
        if (key == "color") {
            if (std::holds_alternative<double>(value.storage()))
                color.setHex(static_cast<int>(value.get<double>()));
            else
                color.copy(Color(value.get<std::string>()));
        } else if (key == "wireframe")
            wireframe = value.get<bool>();
        else if (key == "fog")
            fog = value.get<bool>();
        else if (key == "wireframeLinewidth")
            wireframeLinewidth = value.get<double>();
        else if (key == "wireframeLinecap")
            wireframeLinecap = value.get<std::string>();
        else if (key == "wireframeLinejoin")
            wireframeLinejoin = value.get<std::string>();
        else if (key == "lightMapIntensity")
            lightMapIntensity = value.get<double>();
        else if (key == "aoMapIntensity")
            aoMapIntensity = value.get<double>();
        else if (key == "combine")
            combine = static_cast<int>(value.get<double>());
        else if (key == "reflectivity")
            reflectivity = value.get<double>();
        else if (key == "refractionRatio")
            refractionRatio = value.get<double>();
        else
            base[key] = value;
    }
    Material::setValues(base);
}
Material& MeshBasicMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshBasicMaterial&>(source);
    Material::copy(source);
    color.copy(s.color);
    map = s.map;
    lightMap = s.lightMap;
    lightMapIntensity = s.lightMapIntensity;
    aoMap = s.aoMap;
    aoMapIntensity = s.aoMapIntensity;
    specularMap = s.specularMap;
    alphaMap = s.alphaMap;
    envMap = s.envMap;
    envMapRotation.copy(s.envMapRotation);
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
JsonValue MeshBasicMaterial::toJSON() const {
    if (map || lightMap || aoMap || specularMap || alphaMap || envMap)
        throw std::logic_error("Texture serialization is not ported");
    auto result = Material::toJSON();
    result["color"] = color.getHex();
    result["reflectivity"] = reflectivity;
    result["refractionRatio"] = refractionRatio;
    result["envMapRotation"] =
        JsonValue::Array{envMapRotation.x(), envMapRotation.y(), envMapRotation.z(),
                         envMapRotation.getOrderString()};
    result["wireframe"] = wireframe;
    result["wireframeLinewidth"] = wireframeLinewidth;
    result["wireframeLinecap"] = wireframeLinecap;
    result["wireframeLinejoin"] = wireframeLinejoin;
    result["fog"] = fog;
    return result;
}
