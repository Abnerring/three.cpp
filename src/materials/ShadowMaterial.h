#pragma once
#include "materials/Material.h"

class ShadowMaterial : public Material {
  public:
    bool isShadowMaterial = true;
    Color color{0x000000};
    bool fog = true;
    explicit ShadowMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
