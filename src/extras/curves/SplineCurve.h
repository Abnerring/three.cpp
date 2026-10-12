#pragma once
#include "extras/core/Curve.h"

class SplineCurve : public Curve<Vector2> {
  public:
    static constexpr bool isSplineCurve = true;
    using Points = std::vector<std::shared_ptr<Vector2>>;
    using PointsPtr = std::shared_ptr<Points>;
    PointsPtr points;

    explicit SplineCurve(PointsPtr points = std::make_shared<Points>());
    using Curve<Vector2>::getPoint;
    Vector2& getPoint(double t, Vector2& target) override;
    std::shared_ptr<Curve<Vector2>> clone() const override;
    SplineCurve& copy(const Curve<Vector2>& source) override;
    JsonValue toJSON() const override;
    SplineCurve& fromJSON(const JsonValue& json) override;
};
