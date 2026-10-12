#pragma once
#include "extras/core/Curve.h"

class QuadraticBezierCurve : public Curve<Vector2> {
  public:
    static constexpr bool isQuadraticBezierCurve = true;
    std::shared_ptr<Vector2> v0, v1, v2;

    explicit QuadraticBezierCurve(std::shared_ptr<Vector2> v0 = std::make_shared<Vector2>(),
                           std::shared_ptr<Vector2> v1 = std::make_shared<Vector2>(),
                           std::shared_ptr<Vector2> v2 = std::make_shared<Vector2>());
    using Curve<Vector2>::getPoint;
    Vector2& getPoint(double t, Vector2& target) override;
    std::shared_ptr<Curve<Vector2>> clone() const override;
    QuadraticBezierCurve& copy(const Curve<Vector2>& source) override;
    JsonValue toJSON() const override;
    QuadraticBezierCurve& fromJSON(const JsonValue& json) override;
};
