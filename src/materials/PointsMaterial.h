#pragma once
#include "materials/Material.h"

class PointsMaterial : public Material {
  public:
    bool isPointsMaterial = true;
    Color color{0xffffff};
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> alphaMap;
    double size = 1;
    bool sizeAttenuation = true;
    bool fog = true;
    explicit PointsMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
