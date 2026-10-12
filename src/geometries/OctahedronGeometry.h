#pragma once
#include "geometries/PolyhedronGeometry.h"
class OctahedronGeometry : public PolyhedronGeometry {
  public:
    OctahedronGeometry(double radius = 1, double detail = 0);
    std::shared_ptr<BufferGeometry> clone() const override;
    static std::shared_ptr<OctahedronGeometry> fromJSON(const JsonValue& data);
};
