#pragma once
#include "materials/Material.h"

class MeshBasicMaterial : public Material {
  public:
    Color color{0xffffff};
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> lightMap;
    double lightMapIntensity = 1.0;
    std::shared_ptr<Texture> aoMap;
    double aoMapIntensity = 1.0;
    std::shared_ptr<Texture> specularMap;
    std::shared_ptr<Texture> alphaMap;
    std::shared_ptr<Texture> envMap;
    Euler envMapRotation{};
    int combine = MultiplyOperation;
    double reflectivity = 1;
    double refractionRatio = 0.98;
    bool wireframe = false;
    double wireframeLinewidth = 1;
    std::string wireframeLinecap = "round";
    std::string wireframeLinejoin = "round";
    bool fog = true;
    explicit MeshBasicMaterial(const JsonValue& parameters = JsonValue());
    explicit MeshBasicMaterial(const Color& color);
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
