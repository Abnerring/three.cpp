#pragma once
#include "extras/core/Curve.h"
#include <numbers>

class EllipseCurve : public Curve<Vector2> {
  public:
    static constexpr bool isEllipseCurve = true;
    double aX, aY, xRadius, yRadius, aStartAngle, aEndAngle;
    bool aClockwise;
    double aRotation;

    explicit EllipseCurve(double aX = 0, double aY = 0, double xRadius = 1, double yRadius = 1,
                          double aStartAngle = 0, double aEndAngle = std::numbers::pi * 2,
                          bool aClockwise = false, double aRotation = 0);
    using Curve<Vector2>::getPoint;
    Vector2& getPoint(double t, Vector2& target) override;
    std::shared_ptr<Curve<Vector2>> clone() const override;
    EllipseCurve& copy(const Curve<Vector2>& source) override;
    JsonValue toJSON() const override;
    EllipseCurve& fromJSON(const JsonValue& json) override;
};
