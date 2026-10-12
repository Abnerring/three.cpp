#pragma once
#include "geometries/PolyhedronGeometry.h"
class TetrahedronGeometry : public PolyhedronGeometry {
  public:
    TetrahedronGeometry(double radius = 1, double detail = 0);
    std::shared_ptr<BufferGeometry> clone() const override;
    static std::shared_ptr<TetrahedronGeometry> fromJSON(const JsonValue& data);
};
