#pragma once
#include "core/BufferGeometry.h"
#include <numbers>
#include "math/Vector2.h"
class LatheGeometry : public BufferGeometry {
  public:
    using Points = std::vector<std::shared_ptr<Vector2>>;
    using PointsPtr = std::shared_ptr<Points>;
    static PointsPtr defaultPoints();
    struct Parameters {
        PointsPtr points;
        double segments;
        double phiStart;
        double phiLength;
    };
    Parameters parameters;
    LatheGeometry(PointsPtr points = defaultPoints(), double segments = 12, double phiStart = 0, double phiLength = std::numbers::pi * 2);
    LatheGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<LatheGeometry> fromJSON(const JsonValue& data);
};
