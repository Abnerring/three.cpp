#pragma once
#include "extras/core/Curve.h"

class CubicBezierCurve3 : public Curve<Vector3> {
  public:
    static constexpr bool isCubicBezierCurve3 = true;
    std::shared_ptr<Vector3> v0, v1, v2, v3;

    explicit CubicBezierCurve3(std::shared_ptr<Vector3> v0 = std::make_shared<Vector3>(),
                           std::shared_ptr<Vector3> v1 = std::make_shared<Vector3>(),
                           std::shared_ptr<Vector3> v2 = std::make_shared<Vector3>(),
                           std::shared_ptr<Vector3> v3 = std::make_shared<Vector3>());
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& target) override;
    std::shared_ptr<Curve<Vector3>> clone() const override;
    CubicBezierCurve3& copy(const Curve<Vector3>& source) override;
    JsonValue toJSON() const override;
    CubicBezierCurve3& fromJSON(const JsonValue& json) override;
};
