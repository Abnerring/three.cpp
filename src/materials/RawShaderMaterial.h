#pragma once
#include "materials/ShaderMaterial.h"

class RawShaderMaterial : public ShaderMaterial {
  public:
    bool isRawShaderMaterial = true;
    explicit RawShaderMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    std::shared_ptr<Material> clone() const override;
};
