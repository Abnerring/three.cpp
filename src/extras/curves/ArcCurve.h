#pragma once
#include "extras/curves/EllipseCurve.h"

class ArcCurve : public EllipseCurve {
  public:
    static constexpr bool isArcCurve = true;
    explicit ArcCurve(double aX = 0, double aY = 0, double aRadius = 1, double aStartAngle = 0,
                      double aEndAngle = std::numbers::pi * 2, bool aClockwise = false);
    std::shared_ptr<Curve<Vector2>> clone() const override;
};
