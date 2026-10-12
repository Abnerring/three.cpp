#pragma once
#include "materials/Material.h"

class MeshStandardMaterial : public Material {
  public:
    bool isMeshStandardMaterial = true;
    JSValue defines = JSValue::fromJSON(JsonValue::Object{{"STANDARD",""}});
    Color color{0xffffff};
    double roughness = 1.0;
    double metalness = 0.0;
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> lightMap;
    double lightMapIntensity = 1.0;
    std::shared_ptr<Texture> aoMap;
    double aoMapIntensity = 1.0;
    Color emissive{0x000000};
    double emissiveIntensity = 1.0;
    std::shared_ptr<Texture> emissiveMap;
    std::shared_ptr<Texture> bumpMap;
    double bumpScale = 1;
    std::shared_ptr<Texture> normalMap;
    int normalMapType = TangentSpaceNormalMap;
    Vector2 normalScale{1, 1};
    std::shared_ptr<Texture> displacementMap;
    double displacementScale = 1;
    double displacementBias = 0;
    std::shared_ptr<Texture> roughnessMap;
    std::shared_ptr<Texture> metalnessMap;
    std::shared_ptr<Texture> alphaMap;
    std::shared_ptr<Texture> envMap;
    Euler envMapRotation{};
    double envMapIntensity = 1.0;
    bool wireframe = false;
    double wireframeLinewidth = 1;
    std::string wireframeLinecap = "round";
    std::string wireframeLinejoin = "round";
    bool flatShading = false;
    bool fog = true;
    explicit MeshStandardMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
