#pragma once
#include "materials/Material.h"

class MeshNormalMaterial : public Material {
  public:
    bool isMeshNormalMaterial = true;
    std::shared_ptr<Texture> bumpMap;
    double bumpScale = 1;
    std::shared_ptr<Texture> normalMap;
    int normalMapType = TangentSpaceNormalMap;
    Vector2 normalScale{1, 1};
    std::shared_ptr<Texture> displacementMap;
    double displacementScale = 1;
    double displacementBias = 0;
    bool wireframe = false;
    double wireframeLinewidth = 1;
    bool flatShading = false;
    explicit MeshNormalMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
