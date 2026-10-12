#pragma once
#include "geometries/PolyhedronGeometry.h"
class DodecahedronGeometry : public PolyhedronGeometry {
  public:
    DodecahedronGeometry(double radius = 1, double detail = 0);
    std::shared_ptr<BufferGeometry> clone() const override;
    static std::shared_ptr<DodecahedronGeometry> fromJSON(const JsonValue& data);
};
