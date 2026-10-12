#pragma once
#include "materials/MeshStandardMaterial.h"
#include <limits>

class MeshPhysicalMaterial : public MeshStandardMaterial {
  public:
    bool isMeshPhysicalMaterial = true;
    double anisotropyRotation = 0;
    std::shared_ptr<Texture> anisotropyMap;
    std::shared_ptr<Texture> clearcoatMap;
    double clearcoatRoughness = 0.0;
    std::shared_ptr<Texture> clearcoatRoughnessMap;
    Vector2 clearcoatNormalScale{1, 1};
    std::shared_ptr<Texture> clearcoatNormalMap;
    std::shared_ptr<Texture> diffuseRoughnessMap;
    double ior = 1.5;
    std::shared_ptr<Texture> iridescenceMap;
    double iridescenceIOR = 1.3;
    JSValue iridescenceThicknessRange = JSValue::fromJSON(JsonValue::Array{100, 400});
    std::shared_ptr<Texture> iridescenceThicknessMap;
    Color sheenColor{0x000000};
    std::shared_ptr<Texture> sheenColorMap;
    double sheenRoughness = 1.0;
    std::shared_ptr<Texture> sheenRoughnessMap;
    std::shared_ptr<Texture> transmissionMap;
    double thickness = 0;
    std::shared_ptr<Texture> thicknessMap;
    double attenuationDistance = std::numeric_limits<double>::infinity();
    Color attenuationColor{1, 1, 1};
    double specularIntensity = 1.0;
    std::shared_ptr<Texture> specularIntensityMap;
    Color specularColor{1, 1, 1};
    std::shared_ptr<Texture> specularColorMap;
    AlphaTest anisotropy{&version};
    AlphaTest clearcoat{&version};
    AlphaTest diffuseRoughness{&version};
    AlphaTest dispersion{&version};
    AlphaTest iridescence{&version};
    AlphaTest retroreflectivity{&version};
    AlphaTest sheen{&version};
    AlphaTest transmission{&version};
    Reflectivity reflectivity{&ior};
    explicit MeshPhysicalMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
