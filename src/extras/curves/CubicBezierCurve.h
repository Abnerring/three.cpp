#pragma once
#include "extras/core/Curve.h"

class CubicBezierCurve : public Curve<Vector2> {
  public:
    static constexpr bool isCubicBezierCurve = true;
    std::shared_ptr<Vector2> v0, v1, v2, v3;

    explicit CubicBezierCurve(std::shared_ptr<Vector2> v0 = std::make_shared<Vector2>(),
                           std::shared_ptr<Vector2> v1 = std::make_shared<Vector2>(),
                           std::shared_ptr<Vector2> v2 = std::make_shared<Vector2>(),
                           std::shared_ptr<Vector2> v3 = std::make_shared<Vector2>());
    using Curve<Vector2>::getPoint;
    Vector2& getPoint(double t, Vector2& target) override;
    std::shared_ptr<Curve<Vector2>> clone() const override;
    CubicBezierCurve& copy(const Curve<Vector2>& source) override;
    JsonValue toJSON() const override;
    CubicBezierCurve& fromJSON(const JsonValue& json) override;
};
