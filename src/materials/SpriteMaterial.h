#pragma once
#include "materials/Material.h"

class SpriteMaterial : public Material {
  public:
    bool isSpriteMaterial = true;
    Color color{0xffffff};
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> alphaMap;
    double rotation = 0;
    bool sizeAttenuation = true;
    bool fog = true;
    explicit SpriteMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
