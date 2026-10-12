#pragma once
#include "materials/Material.h"

class LineBasicMaterial : public Material {
  public:
    bool isLineBasicMaterial = true;
    Color color{0xffffff};
    std::shared_ptr<Texture> map;
    double linewidth = 1;
    std::string linecap = "round";
    std::string linejoin = "round";
    bool fog = true;
    explicit LineBasicMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
