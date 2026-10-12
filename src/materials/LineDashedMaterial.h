#pragma once
#include "materials/LineBasicMaterial.h"

class LineDashedMaterial : public LineBasicMaterial {
  public:
    bool isLineDashedMaterial = true;
    double scale = 1;
    double dashSize = 3;
    double gapSize = 1;
    explicit LineDashedMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
};
