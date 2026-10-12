#pragma once
#include "materials/Material.h"

class MeshToonMaterial : public Material {
  public:
    bool isMeshToonMaterial = true;
    JSValue defines = JSValue::fromJSON(JsonValue::Object{{"TOON",""}});
    Color color{0xffffff};
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> gradientMap;
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
    std::shared_ptr<Texture> alphaMap;
    bool wireframe = false;
    double wireframeLinewidth = 1;
    std::string wireframeLinecap = "round";
    std::string wireframeLinejoin = "round";
    bool fog = true;
    explicit MeshToonMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
