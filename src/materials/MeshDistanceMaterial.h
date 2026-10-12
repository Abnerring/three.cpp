#pragma once
#include "materials/Material.h"

class MeshDistanceMaterial : public Material {
  public:
    bool isMeshDistanceMaterial = true;
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> alphaMap;
    std::shared_ptr<Texture> displacementMap;
    double displacementScale = 1;
    double displacementBias = 0;
    explicit MeshDistanceMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
