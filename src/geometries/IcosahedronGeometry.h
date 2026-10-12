#pragma once
#include "geometries/PolyhedronGeometry.h"
class IcosahedronGeometry : public PolyhedronGeometry {
  public:
    IcosahedronGeometry(double radius = 1, double detail = 0);
    std::shared_ptr<BufferGeometry> clone() const override;
    static std::shared_ptr<IcosahedronGeometry> fromJSON(const JsonValue& data);
};
