#pragma once
#include "materials/Material.h"

class MeshMatcapMaterial : public Material {
  public:
    bool isMeshMatcapMaterial = true;
    JSValue defines = JSValue::fromJSON(JsonValue::Object{{"MATCAP",""}});
    Color color{0xffffff};
    std::shared_ptr<Texture> matcap;
    std::shared_ptr<Texture> map;
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
    bool flatShading = false;
    bool fog = true;
    explicit MeshMatcapMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
