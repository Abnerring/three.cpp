#pragma once
#include "materials/Material.h"

class MeshDepthMaterial : public Material {
  public:
    bool isMeshDepthMaterial = true;
    int depthPacking = BasicDepthPacking;
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> alphaMap;
    std::shared_ptr<Texture> displacementMap;
    double displacementScale = 1;
    double displacementBias = 0;
    bool wireframe = false;
    double wireframeLinewidth = 1;
    explicit MeshDepthMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
