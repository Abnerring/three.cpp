#pragma once
#include "materials/Material.h"

class MeshPhongMaterial : public Material {
  public:
    bool isMeshPhongMaterial = true;
    Color color{0xffffff};
    Color specular{0x111111};
    double shininess = 30;
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
    std::shared_ptr<Texture> specularMap;
    std::shared_ptr<Texture> alphaMap;
    std::shared_ptr<Texture> envMap;
    Euler envMapRotation{};
    int combine = MultiplyOperation;
    double reflectivity = 1;
    double envMapIntensity = 1.0;
    double refractionRatio = 0.98;
    bool wireframe = false;
    double wireframeLinewidth = 1;
    std::string wireframeLinecap = "round";
    std::string wireframeLinejoin = "round";
    bool flatShading = false;
    bool fog = true;
    explicit MeshPhongMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
