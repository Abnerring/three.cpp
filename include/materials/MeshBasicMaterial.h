#pragma once
#include "materials/Material.h"
#include "math/Euler.h"
class Texture;
class MeshBasicMaterial : public Material {
  public:
    Color color;
    std::shared_ptr<Texture> map, lightMap, aoMap, specularMap, alphaMap, envMap;
    double lightMapIntensity = 1, aoMapIntensity = 1;
    Euler envMapRotation;
    int combine = MultiplyOperation;
    double reflectivity = 1, refractionRatio = .98;
    bool wireframe = false;
    double wireframeLinewidth = 1;
    std::string wireframeLinecap = "round", wireframeLinejoin = "round";
    bool fog = true;
    explicit MeshBasicMaterial(const JsonValue& parameters = JsonValue());
    explicit MeshBasicMaterial(const Color& color);
    void setValues(const JsonValue& parameters) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
    JsonValue toJSON() const override;
};
