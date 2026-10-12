#include "materials/MeshPhysicalMaterial.h"

MeshPhysicalMaterial::MeshPhysicalMaterial(const JsonValue& parameters) {
    defines = JSValue::fromJSON(JsonValue::Object{{"STANDARD", ""}, {"PHYSICAL", ""}});
    type = "MeshPhysicalMaterial";
    setValues(parameters);
}
void MeshPhysicalMaterial::visitProperties(const PropertyVisitor& visitor) {
    MeshStandardMaterial::visitProperties(visitor);
    visitor("isMeshPhysicalMaterial", &isMeshPhysicalMaterial);
    visitor("anisotropyRotation", &anisotropyRotation);
    visitor("anisotropyMap", &anisotropyMap);
    visitor("clearcoatMap", &clearcoatMap);
    visitor("clearcoatRoughness", &clearcoatRoughness);
    visitor("clearcoatRoughnessMap", &clearcoatRoughnessMap);
    visitor("clearcoatNormalScale", &clearcoatNormalScale);
    visitor("clearcoatNormalMap", &clearcoatNormalMap);
    visitor("diffuseRoughnessMap", &diffuseRoughnessMap);
    visitor("ior", &ior);
    visitor("reflectivity", &reflectivity);
    visitor("iridescenceMap", &iridescenceMap);
    visitor("iridescenceIOR", &iridescenceIOR);
    visitor("iridescenceThicknessRange", &iridescenceThicknessRange);
    visitor("iridescenceThicknessMap", &iridescenceThicknessMap);
    visitor("sheenColor", &sheenColor);
    visitor("sheenColorMap", &sheenColorMap);
    visitor("sheenRoughness", &sheenRoughness);
    visitor("sheenRoughnessMap", &sheenRoughnessMap);
    visitor("transmissionMap", &transmissionMap);
    visitor("thickness", &thickness);
    visitor("thicknessMap", &thicknessMap);
    visitor("attenuationDistance", &attenuationDistance);
    visitor("attenuationColor", &attenuationColor);
    visitor("specularIntensity", &specularIntensity);
    visitor("specularIntensityMap", &specularIntensityMap);
    visitor("specularColor", &specularColor);
    visitor("specularColorMap", &specularColorMap);
    visitor("_anisotropy", &anisotropy.value);
    visitor("anisotropy", &anisotropy);
    visitor("_clearcoat", &clearcoat.value);
    visitor("clearcoat", &clearcoat);
    visitor("_diffuseRoughness", &diffuseRoughness.value);
    visitor("diffuseRoughness", &diffuseRoughness);
    visitor("_dispersion", &dispersion.value);
    visitor("dispersion", &dispersion);
    visitor("_iridescence", &iridescence.value);
    visitor("iridescence", &iridescence);
    visitor("_retroreflectivity", &retroreflectivity.value);
    visitor("retroreflectivity", &retroreflectivity);
    visitor("_sheen", &sheen.value);
    visitor("sheen", &sheen);
    visitor("_transmission", &transmission.value);
    visitor("transmission", &transmission);
}
Material& MeshPhysicalMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshPhysicalMaterial&>(source);
    MeshStandardMaterial::copy(source);
    defines = JSValue::fromJSON(JsonValue::Object{{"STANDARD", ""}, {"PHYSICAL", ""}});
    anisotropy = s.anisotropy;
    anisotropyRotation = s.anisotropyRotation;
    anisotropyMap = s.anisotropyMap;
    clearcoat = s.clearcoat;
    clearcoatMap = s.clearcoatMap;
    clearcoatRoughness = s.clearcoatRoughness;
    clearcoatRoughnessMap = s.clearcoatRoughnessMap;
    clearcoatNormalMap = s.clearcoatNormalMap;
    clearcoatNormalScale.copy( s.clearcoatNormalScale );
    diffuseRoughness = s.diffuseRoughness;
    diffuseRoughnessMap = s.diffuseRoughnessMap;
    dispersion = s.dispersion;
    ior = s.ior;
    iridescence = s.iridescence;
    iridescenceMap = s.iridescenceMap;
    iridescenceIOR = s.iridescenceIOR;
    const auto sourceRange = s.iridescenceThicknessRange;
    auto copiedRange = JSValue::array(sourceRange.length());
    for (std::uint32_t i = 0; i < copiedRange.length(); ++i)
        copiedRange.set(std::to_string(i), sourceRange.get(std::to_string(i)));
    iridescenceThicknessRange = std::move(copiedRange);
    iridescenceThicknessMap = s.iridescenceThicknessMap;
    retroreflectivity = s.retroreflectivity;
    sheen = s.sheen;
    sheenColor.copy( s.sheenColor );
    sheenColorMap = s.sheenColorMap;
    sheenRoughness = s.sheenRoughness;
    sheenRoughnessMap = s.sheenRoughnessMap;
    transmission = s.transmission;
    transmissionMap = s.transmissionMap;
    thickness = s.thickness;
    thicknessMap = s.thicknessMap;
    attenuationDistance = s.attenuationDistance;
    attenuationColor.copy( s.attenuationColor );
    specularIntensity = s.specularIntensity;
    specularIntensityMap = s.specularIntensityMap;
    specularColor.copy( s.specularColor );
    specularColorMap = s.specularColorMap;
    return *this;
}
std::shared_ptr<Material> MeshPhysicalMaterial::clone() const {
    auto result = std::make_shared<MeshPhysicalMaterial>();
    result->copy(*this);
    return result;
}
